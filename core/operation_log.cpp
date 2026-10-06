#include "operation_log.h"

#include <ctime>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <utility>

using namespace std;

namespace
{
bool isOperationTime(const string &value)
{
    if (value.size() != 19 ||
        value[4] != '-' || value[7] != '-' || value[10] != 'T' ||
        value[13] != ':' || value[16] != ':')
    {
        return false;
    }

    for (size_t index = 0; index < value.size(); ++index)
    {
        if (index == 4 || index == 7 || index == 10 ||
            index == 13 || index == 16)
        {
            continue;
        }
        if (value[index] < '0' || value[index] > '9')
        {
            return false;
        }
    }

    const int year = stoi(value.substr(0, 4));
    const int month = stoi(value.substr(5, 2));
    const int day = stoi(value.substr(8, 2));
    const int hour = stoi(value.substr(11, 2));
    const int minute = stoi(value.substr(14, 2));
    const int second = stoi(value.substr(17, 2));

    if (year < 1 || month < 1 || month > 12 ||
        hour > 23 || minute > 59 || second > 59)
    {
        return false;
    }

    const bool leapYear =
        (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
    const int daysByMonth[] = {
        31, leapYear ? 29 : 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31};
    return day >= 1 && day <= daysByMonth[month - 1];
}

string encodeField(const string &value)
{
    if (value.find_first_of(",\"\r\n") == string::npos)
    {
        return value;
    }

    string encoded = "\"";
    for (char character : value)
    {
        if (character == '"')
        {
            encoded += "\"\"";
        }
        else
        {
            encoded += character;
        }
    }
    encoded += '"';
    return encoded;
}

bool parseCsvRows(
    const string &csv,
    vector<vector<string>> &rows)
{
    vector<vector<string>> parsedRows;
    vector<string> fields;
    string field;
    bool insideQuotes = false;
    bool afterQuote = false;
    bool recordHasContent = false;

    const auto finishRecord = [&]()
    {
        fields.push_back(field);
        parsedRows.push_back(fields);
        fields.clear();
        field.clear();
        afterQuote = false;
        recordHasContent = false;
    };

    for (size_t index = 0; index < csv.size(); ++index)
    {
        const char character = csv[index];
        if (insideQuotes)
        {
            if (character == '"')
            {
                if (index + 1 < csv.size() && csv[index + 1] == '"')
                {
                    field += '"';
                    ++index;
                }
                else
                {
                    insideQuotes = false;
                    afterQuote = true;
                }
            }
            else
            {
                field += character;
            }
            continue;
        }

        if (afterQuote)
        {
            if (character == ',')
            {
                fields.push_back(field);
                field.clear();
                afterQuote = false;
                recordHasContent = true;
                continue;
            }
            if (character != '\r' && character != '\n')
            {
                return false;
            }
        }
        else if (character == '"')
        {
            if (!field.empty())
            {
                return false;
            }
            insideQuotes = true;
            recordHasContent = true;
            continue;
        }
        else if (character == ',')
        {
            fields.push_back(field);
            field.clear();
            recordHasContent = true;
            continue;
        }
        else if (character != '\r' && character != '\n')
        {
            field += character;
            recordHasContent = true;
            continue;
        }

        finishRecord();
        if (character == '\r' && index + 1 < csv.size() &&
            csv[index + 1] == '\n')
        {
            ++index;
        }
    }

    if (insideQuotes)
    {
        return false;
    }
    if (recordHasContent || afterQuote || !fields.empty() || !field.empty())
    {
        finishRecord();
    }

    rows.swap(parsedRows);
    return !rows.empty();
}
}

string operationTypeToken(OperationType type)
{
    switch (type)
    {
    case OperationType::VolunteerRecordApproved:
        return "VolunteerRecordApproved";
    case OperationType::VolunteerRecordRejected:
        return "VolunteerRecordRejected";
    }
    return "";
}

bool parseOperationTypeToken(const string &token, OperationType &type)
{
    if (token == "VolunteerRecordApproved")
    {
        type = OperationType::VolunteerRecordApproved;
        return true;
    }
    if (token == "VolunteerRecordRejected")
    {
        type = OperationType::VolunteerRecordRejected;
        return true;
    }
    return false;
}

string operationTargetTypeToken(OperationTargetType type)
{
    if (type == OperationTargetType::VolunteerRecord)
    {
        return "VolunteerRecord";
    }
    return "";
}

bool parseOperationTargetTypeToken(
    const string &token,
    OperationTargetType &type)
{
    if (token != "VolunteerRecord")
    {
        return false;
    }
    type = OperationTargetType::VolunteerRecord;
    return true;
}

bool parseOperationLogSequence(
    const string &logId,
    unsigned long long &sequence)
{
    if (logId.size() < 9 || logId.compare(0, 3, "LOG") != 0)
    {
        return false;
    }

    unsigned long long value = 0;
    for (size_t index = 3; index < logId.size(); ++index)
    {
        const char digit = logId[index];
        if (digit < '0' || digit > '9')
        {
            return false;
        }
        const unsigned int number = static_cast<unsigned int>(digit - '0');
        if (value > (numeric_limits<unsigned long long>::max() - number) / 10)
        {
            return false;
        }
        value = value * 10 + number;
    }
    sequence = value;
    return value > 0;
}

string currentLocalOperationTime()
{
    const time_t currentTime = time(nullptr);
    const tm *localTime = localtime(&currentTime);
    if (localTime == nullptr)
    {
        throw runtime_error("could not read local operation time");
    }

    char formatted[20] = {};
    if (strftime(formatted, sizeof(formatted), "%Y-%m-%dT%H:%M:%S", localTime) == 0)
    {
        throw runtime_error("could not format local operation time");
    }
    return formatted;
}

OperationLog::OperationLog(
    string logId,
    string operatorAccountId,
    OperationType operationType,
    OperationTargetType targetType,
    string targetId,
    string description,
    string operationTime)
    : logId_(move(logId)),
      operatorAccountId_(move(operatorAccountId)),
      operationType_(operationType),
      targetType_(targetType),
      targetId_(move(targetId)),
      description_(move(description)),
      operationTime_(move(operationTime))
{
    unsigned long long sequence = 0;
    if (!parseOperationLogSequence(logId_, sequence) ||
        operatorAccountId_.empty() || targetId_.empty() ||
        operationTypeToken(operationType_).empty() ||
        operationTargetTypeToken(targetType_).empty() ||
        !isOperationTime(operationTime_))
    {
        throw invalid_argument("invalid OperationLog field");
    }
}

const string &OperationLog::getLogId() const
{
    return logId_;
}

const string &OperationLog::getOperatorAccountId() const
{
    return operatorAccountId_;
}

OperationType OperationLog::getOperationType() const
{
    return operationType_;
}

OperationTargetType OperationLog::getTargetType() const
{
    return targetType_;
}

const string &OperationLog::getTargetId() const
{
    return targetId_;
}

const string &OperationLog::getDescription() const
{
    return description_;
}

const string &OperationLog::getOperationTime() const
{
    return operationTime_;
}

const string &OperationLogCsvCodec::header()
{
    static const string value =
        "logId,operatorAccountId,operationType,targetType,targetId,description,operationTime";
    return value;
}

string OperationLogCsvCodec::serialize(const vector<OperationLog> &logs)
{
    string csv = header() + "\n";
    for (const OperationLog &log : logs)
    {
        const vector<string> fields = {
            log.getLogId(),
            log.getOperatorAccountId(),
            operationTypeToken(log.getOperationType()),
            operationTargetTypeToken(log.getTargetType()),
            log.getTargetId(),
            log.getDescription(),
            log.getOperationTime()};

        for (size_t index = 0; index < fields.size(); ++index)
        {
            if (index > 0)
            {
                csv += ',';
            }
            csv += encodeField(fields[index]);
        }
        csv += '\n';
    }
    return csv;
}

bool OperationLogCsvCodec::parse(
    const string &csv,
    vector<OperationLog> &logs)
{
    vector<vector<string>> rows;
    if (!parseCsvRows(csv, rows) || rows.front().size() != 7)
    {
        return false;
    }

    const vector<string> expectedHeader = {
        "logId", "operatorAccountId", "operationType", "targetType",
        "targetId", "description", "operationTime"};
    if (rows.front() != expectedHeader)
    {
        return false;
    }

    vector<OperationLog> parsedLogs;
    unordered_set<string> logIds;
    for (size_t rowIndex = 1; rowIndex < rows.size(); ++rowIndex)
    {
        const vector<string> &fields = rows[rowIndex];
        if (fields.size() != 7)
        {
            return false;
        }

        OperationType operationType;
        OperationTargetType targetType;
        if (!parseOperationTypeToken(fields[2], operationType) ||
            !parseOperationTargetTypeToken(fields[3], targetType))
        {
            return false;
        }
        if (!logIds.insert(fields[0]).second)
        {
            return false;
        }

        try
        {
            parsedLogs.emplace_back(
                fields[0], fields[1], operationType, targetType,
                fields[4], fields[5], fields[6]);
        }
        catch (const invalid_argument &)
        {
            return false;
        }
    }

    logs.swap(parsedLogs);
    return true;
}
