#include "diary_post.h"
using namespace std;

DiaryPost::DiaryPost(const string &diaryId, const string &studentId, const string &recordId, const string &message, int likeCount) : diaryId(diaryId), studentId(studentId), recordId(recordId), message(message), likeCount(likeCount) {}
string DiaryPost::getDiaryId() const
{
    return diaryId;
}
string DiaryPost::getStudentId() const
{
    return studentId;
}
string DiaryPost::getRecordId() const
{
    return recordId;
}
string DiaryPost::getMessage() const
{
    return message;
}
int DiaryPost::getLikeCount() const
{
    return likeCount;
}
const vector<string> &DiaryPost::getLikedStudentIds() const
{
    return likedStudentIds;
}
bool DiaryPost::hasLiked(const string &studentId) const
{
    for (const string &id : likedStudentIds)
    {
        if (id == studentId)
        {
            return true;
        }
    }
    return false;
}

bool DiaryPost::addLike(const string &studentId)
{
    if (hasLiked(studentId))
    {
        return false;
    }
    likedStudentIds.push_back(studentId);
    ++likeCount;
    return true;
}
void DiaryPost::addLikedStudentId(const string &studentId)
{
    if (!hasLiked(studentId))
    {
        likedStudentIds.push_back(studentId);
    }
}