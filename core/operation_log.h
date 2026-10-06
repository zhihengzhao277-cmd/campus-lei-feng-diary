#ifndef OPERATION_LOG_H
#define OPERATION_LOG_H

#include <string>
#include <vector>

enum class OperationType
{
    VolunteerRecordApproved,
    VolunteerRecordRejected
};

enum class OperationTargetType
{
    VolunteerRecord
};

std::string operationTypeToken(OperationType type);
bool parseOperationTypeToken(
    const std::string &token,
    OperationType &type);

std::string operationTargetTypeToken(OperationTargetType type);
bool parseOperationTargetTypeToken(
    const std::string &token,
    OperationTargetType &type);

bool parseOperationLogSequence(
    const std::string &logId,
    unsigned long long &sequence);

std::string currentLocalOperationTime();

class OperationLog
{
public:
    OperationLog(
        std::string logId,
        std::string operatorAccountId,
        OperationType operationType,
        OperationTargetType targetType,
        std::string targetId,
        std::string description,
        std::string operationTime);

    const std::string &getLogId() const;
    const std::string &getOperatorAccountId() const;
    OperationType getOperationType() const;
    OperationTargetType getTargetType() const;
    const std::string &getTargetId() const;
    const std::string &getDescription() const;
    const std::string &getOperationTime() const;

private:
    const std::string logId_;
    const std::string operatorAccountId_;
    const OperationType operationType_;
    const OperationTargetType targetType_;
    const std::string targetId_;
    const std::string description_;
    const std::string operationTime_;
};

class OperationLogCsvCodec
{
public:
    static const std::string &header();
    static std::string serialize(
        const std::vector<OperationLog> &logs);
    static bool parse(
        const std::string &csv,
        std::vector<OperationLog> &logs);
};

#endif
