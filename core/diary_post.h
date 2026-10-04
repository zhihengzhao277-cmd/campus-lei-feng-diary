#ifndef DIARY_POST_H
#define DIARY_POST_H
#include <string>
#include <vector>
using namespace std;

class DiaryPost
{
private:
    string diaryId;
    string studentId;
    string recordId;
    string message;
    int likeCount;
    vector<string> likedStudentIds;

public:
    DiaryPost(const string &diaryId, const string &studentId, const string &recordId, const string &messag, int likeCount = 0);
    string getDiaryId() const;
    string getStudentId() const;
    string getRecordId() const;
    string getMessage() const;
    int getLikeCount() const;
    const vector<string> &getLikedStudentIds() const;
    bool hasLiked(const string &studentId) const;
    bool addLike(const string &studentId);
    void addLikedStudentId(const string &studentId);
};
#endif