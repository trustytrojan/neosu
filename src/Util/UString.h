#pragma once
#include <algorithm>
#include <cwctype>
#include <string>
#include <vector>

class UString;

void trim(std::string *str);

struct MD5Hash {
    char hash[33];

    MD5Hash(const char *str);
    MD5Hash() { hash[0] = 0; }

    inline const char *toUtf8() const { return hash; }
    bool operator==(const MD5Hash &other) const;
    bool operator==(const UString &other) const;
};

class UString {
   public:
    static UString format(const char *utf8format, ...);

   public:
    UString();
    UString(const wchar_t *str);
    UString(const char *utf8);
    UString(const char *utf8, int length);
    UString(const UString &ustr);
    UString(UString &&ustr);
    ~UString();

    void clear();

    // get
    operator char *() const { return mUtf8; }
    operator wchar_t *() const { return mUnicode; }
    inline int length() const { return mLength; }
    inline int lengthUtf8() const { return mLengthUtf8; }
    inline const char *toUtf8() const { return mUtf8; }
    inline const wchar_t *wc_str() const { return mUnicode; }
    inline bool isAsciiOnly() const { return mIsAsciiOnly; }
    bool isWhitespaceOnly() const;

    int findChar(wchar_t ch, int start = 0, bool respectEscapeChars = false) const;
    int findChar(const UString &str, int start = 0, bool respectEscapeChars = false) const;
    int find(const UString &str, int start = 0) const;
    int find(const UString &str, int start, int end) const;
    int findLast(const UString &str, int start = 0) const;
    int findLast(const UString &str, int start, int end) const;

    int findIgnoreCase(const UString &str, int start = 0) const;
    int findIgnoreCase(const UString &str, int start, int end) const;

    // modifiers
    void collapseEscapes();
    void append(const UString &str);
    void append(wchar_t ch);
    void insert(int offset, const UString &str);
    void insert(int offset, wchar_t ch);
    void erase(int offset, int count);

    // actions (non-modifying)
    UString substr(int offset, int charCount = -1) const;
    std::vector<UString> split(UString delim) const;
    UString trim() const;

    // conversions
    float toFloat() const;
    double toDouble() const;
    long double toLongDouble() const;
    int toInt() const;
    long toLong() const;
    long long toLongLong() const;
    unsigned int toUnsignedInt() const;
    unsigned long toUnsignedLong() const;
    unsigned long long toUnsignedLongLong() const;

    void lowerCase();
    void upperCase();

    // operators
    wchar_t operator[](int index) const;
    UString &operator=(const UString &ustr);
    UString &operator=(UString &&ustr);
    bool operator==(const UString &ustr) const;
    bool operator!=(const UString &ustr) const;
    bool operator<(const UString &ustr) const;

    bool equalsIgnoreCase(const UString &ustr) const;
    bool lessThanIgnoreCase(const UString &ustr) const;

   private:
    static int decode(const char *utf8, wchar_t *unicode, int utf8Length);
    static int encode(const wchar_t *unicode, int length, char *utf8, bool *isAsciiOnly);

    static wchar_t getCodePoint(const char *utf8, int offset, int numBytes, unsigned char firstByteMask);

    static void getUtf8(wchar_t ch, char *utf8, int numBytes, int firstByteValue);

    int fromUtf8(const char *utf8, int length = -1);

    void updateUtf8();

    // inline deletes, guarantee valid empty string
    inline void deleteUnicode() {
        if(mUnicode != NULL && mUnicode != nullWString) delete[] mUnicode;

        mUnicode = (wchar_t *)nullWString;
    }

    inline void deleteUtf8() {
        if(mUtf8 != NULL && mUtf8 != nullString) delete[] mUtf8;

        mUtf8 = (char *)nullString;
    }

    inline bool isUnicodeNull() const { return (mUnicode == NULL || mUnicode == nullWString); }
    inline bool isUtf8Null() const { return (mUtf8 == NULL || mUtf8 == nullString); }

   private:
    static constexpr char nullString[] = "";
    static constexpr wchar_t nullWString[] = L"";

    int mLength;
    int mLengthUtf8;
    bool mIsAsciiOnly;

    wchar_t *mUnicode;
    char *mUtf8;
};

namespace std {
template <>
struct hash<UString> {
    size_t operator()(const UString &str) const {
        std::string tmp(str.toUtf8(), str.lengthUtf8());
        return std::hash<std::string>()(tmp);
    }
};

template <>
struct hash<MD5Hash> {
    size_t operator()(const MD5Hash &md5) const {
        std::string_view tmp(md5.hash, 32);
        return std::hash<std::string_view>()(tmp);
    }
};
}  // namespace std
