#include <gtest/gtest.h>
#include <cstring>
#include <dirent.h>
#include <stdlib.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

extern "C" {
#include "webserver.h"
#include "intern/system_file_access_utils.h"
#include "intern/system_file_access.h"
#include "intern/system_file_cache.h"
}

// File Type Detection Tests (MIME Types)
class FileTypeDetectionTest : public ::testing::Test {
protected:
    WebserverFileInfo* file;

    void SetUp() override {
        file = create_empty_file(0);
    }

    void TearDown() override {
        free_empty_file(file);
    }

    void setUrlAndDetectType(const char* url) {
        copyURL(file, url);
        setFileType(file);
    }
};

TEST_F(FileTypeDetectionTest, HtmlFile) {
    setUrlAndDetectType("/index.html");
    EXPECT_EQ(file->FileType, FILE_TYPE_HTML);
}

TEST_F(FileTypeDetectionTest, CssFile) {
    setUrlAndDetectType("/styles.css");
    EXPECT_EQ(file->FileType, FILE_TYPE_CSS);
}

TEST_F(FileTypeDetectionTest, JsFile) {
    setUrlAndDetectType("/app.js");
    EXPECT_EQ(file->FileType, FILE_TYPE_JS);
}

TEST_F(FileTypeDetectionTest, PngFile) {
    setUrlAndDetectType("/image.png");
    EXPECT_EQ(file->FileType, FILE_TYPE_PNG);
}

TEST_F(FileTypeDetectionTest, JpgFile) {
    setUrlAndDetectType("/photo.jpg");
    EXPECT_EQ(file->FileType, FILE_TYPE_JPG);
}

TEST_F(FileTypeDetectionTest, GifFile) {
    setUrlAndDetectType("/animation.gif");
    EXPECT_EQ(file->FileType, FILE_TYPE_GIF);
}

TEST_F(FileTypeDetectionTest, IcoFile) {
    setUrlAndDetectType("/favicon.ico");
    EXPECT_EQ(file->FileType, FILE_TYPE_ICO);
}

TEST_F(FileTypeDetectionTest, SvgFile) {
    setUrlAndDetectType("/vector.svg");
    EXPECT_EQ(file->FileType, FILE_TYPE_SVG);
}

TEST_F(FileTypeDetectionTest, PdfFile) {
    setUrlAndDetectType("/document.pdf");
    EXPECT_EQ(file->FileType, FILE_TYPE_PDF);
}

TEST_F(FileTypeDetectionTest, JsonFile) {
    setUrlAndDetectType("/data.json");
    EXPECT_EQ(file->FileType, FILE_TYPE_JSON);
}

TEST_F(FileTypeDetectionTest, XmlFile) {
    setUrlAndDetectType("/config.xml");
    EXPECT_EQ(file->FileType, FILE_TYPE_XML);
}

TEST_F(FileTypeDetectionTest, WoffFile) {
    setUrlAndDetectType("/font.woff");
    EXPECT_EQ(file->FileType, FILE_TYPE_WOFF);
}

TEST_F(FileTypeDetectionTest, TtfFile) {
    setUrlAndDetectType("/font.ttf");
    EXPECT_EQ(file->FileType, FILE_TYPE_TTF);
}

TEST_F(FileTypeDetectionTest, EotFile) {
    setUrlAndDetectType("/font.eot");
    EXPECT_EQ(file->FileType, FILE_TYPE_EOT);
}

TEST_F(FileTypeDetectionTest, BmpFile) {
    setUrlAndDetectType("/image.bmp");
    EXPECT_EQ(file->FileType, FILE_TYPE_BMP);
}

TEST_F(FileTypeDetectionTest, ManifestFile) {
    setUrlAndDetectType("/app.manifest");
    EXPECT_EQ(file->FileType, FILE_TYPE_MANIFEST);
}

TEST_F(FileTypeDetectionTest, XslFile) {
    setUrlAndDetectType("/transform.xsl");
    EXPECT_EQ(file->FileType, FILE_TYPE_XSL);
}

TEST_F(FileTypeDetectionTest, IncFile) {
    setUrlAndDetectType("/header.inc");
    EXPECT_EQ(file->FileType, FILE_TYPE_HTML_INC);
}

TEST_F(FileTypeDetectionTest, CSourceFile) {
    setUrlAndDetectType("/main.c");
    EXPECT_EQ(file->FileType, FILE_TYPE_C_SRC);
}

TEST_F(FileTypeDetectionTest, UnknownExtension) {
    setUrlAndDetectType("/file.xyz");
    EXPECT_EQ(file->FileType, FILE_TYPE_PLAIN);
}

TEST_F(FileTypeDetectionTest, NoExtension) {
    setUrlAndDetectType("/README");
    EXPECT_EQ(file->FileType, FILE_TYPE_PLAIN);
}

TEST_F(FileTypeDetectionTest, HiddenFile) {
    setUrlAndDetectType("/.htaccess");
    EXPECT_EQ(file->FileType, FILE_TYPE_PLAIN);
}

TEST_F(FileTypeDetectionTest, MultipleDotsInFilename) {
    setUrlAndDetectType("/file.test.html");
    EXPECT_EQ(file->FileType, FILE_TYPE_HTML);
}

TEST_F(FileTypeDetectionTest, UppercaseExtension) {
    setUrlAndDetectType("/FILE.HTML");
    EXPECT_EQ(file->FileType, FILE_TYPE_HTML);
}

TEST_F(FileTypeDetectionTest, MixedCaseExtension) {
    setUrlAndDetectType("/file.HtMl");
    EXPECT_EQ(file->FileType, FILE_TYPE_HTML);
}

TEST_F(FileTypeDetectionTest, PathWithDirectories) {
    setUrlAndDetectType("/path/to/file.css");
    EXPECT_EQ(file->FileType, FILE_TYPE_CSS);
}

TEST_F(FileTypeDetectionTest, DirectoryWithDot) {
    setUrlAndDetectType("/path.old/to/file");
    // Should treat as no extension
    EXPECT_EQ(file->FileType, FILE_TYPE_PLAIN);
}

// URL Normalization Tests
class UrlNormalizationTest : public ::testing::Test {
protected:
    WebserverFileInfo* file;

    void SetUp() override {
        file = create_empty_file(0);
    }

    void TearDown() override {
        free_empty_file(file);
    }
};

TEST_F(UrlNormalizationTest, StripLeadingSlash) {
    copyURL(file, "/index.html");
    EXPECT_STREQ(file->Url, "index.html");
}

TEST_F(UrlNormalizationTest, StripMultipleLeadingSlashes) {
    copyURL(file, "///index.html");
    EXPECT_STREQ(file->Url, "index.html");
}

TEST_F(UrlNormalizationTest, NoLeadingSlash) {
    copyURL(file, "index.html");
    EXPECT_STREQ(file->Url, "index.html");
}

TEST_F(UrlNormalizationTest, PathWithSubdirectory) {
    copyURL(file, "/path/to/file.html");
    EXPECT_STREQ(file->Url, "path/to/file.html");
}

TEST_F(UrlNormalizationTest, RootUrl) {
    copyURL(file, "/");
    EXPECT_STREQ(file->Url, "");
}

TEST_F(UrlNormalizationTest, EmptyUrl) {
    copyURL(file, "");
    EXPECT_STREQ(file->Url, "");
}

TEST_F(UrlNormalizationTest, UrlLengthCorrect) {
    copyURL(file, "/test.html");
    EXPECT_EQ(file->UrlLengt, strlen("test.html"));
}

// File Path Copy Tests
class FilePathCopyTest : public ::testing::Test {
protected:
    WebserverFileInfo* file;

    void SetUp() override {
        file = create_empty_file(0);
    }

    void TearDown() override {
        free_empty_file(file);
    }
};

TEST_F(FilePathCopyTest, SimpleFilePath) {
    copyFilePath(file, "/var/www/index.html");
    EXPECT_STREQ(file->FilePath, "/var/www/index.html");
}

TEST_F(FilePathCopyTest, FilePathLength) {
    copyFilePath(file, "/var/www/test.html");
    EXPECT_EQ(file->FilePathLengt, strlen("/var/www/test.html"));
}

TEST_F(FilePathCopyTest, OverwriteFilePath) {
    copyFilePath(file, "/first/path.html");
    copyFilePath(file, "/second/path.html");
    EXPECT_STREQ(file->FilePath, "/second/path.html");
}

TEST_F(FilePathCopyTest, EmptyFilePath) {
    copyFilePath(file, "");
    EXPECT_STREQ(file->FilePath, "");
    EXPECT_EQ(file->FilePathLengt, 0u);
}

// File Info Memory Tests
class FileInfoMemoryTest : public ::testing::Test {};

TEST_F(FileInfoMemoryTest, CreateEmptyFile) {
    WebserverFileInfo* file = create_empty_file(0);
    ASSERT_NE(file, nullptr);
    free_empty_file(file);
}

TEST_F(FileInfoMemoryTest, CreateWithSize) {
    WebserverFileInfo* file = create_empty_file(1024);
    ASSERT_NE(file, nullptr);
    // Data should be allocated
    EXPECT_NE(file->Data, nullptr);
    EXPECT_EQ(file->DataLenght, 1024u);
    free_empty_file(file);
}

TEST_F(FileInfoMemoryTest, CreateMultipleFiles) {
    WebserverFileInfo* files[10];

    for (int i = 0; i < 10; i++) {
        files[i] = create_empty_file(100);
        ASSERT_NE(files[i], nullptr);
    }

    for (int i = 0; i < 10; i++) {
        free_empty_file(files[i]);
    }
}

// Cache Header Comparison Tests (ETag/Last-Modified logic)
class CacheComparisonTest : public ::testing::Test {};

TEST_F(CacheComparisonTest, ETagExactMatch) {
    const char* etag1 = "\"abc123\"";
    const char* etag2 = "\"abc123\"";

    EXPECT_EQ(strcmp(etag1, etag2), 0);
}

TEST_F(CacheComparisonTest, ETagMismatch) {
    const char* etag1 = "\"abc123\"";
    const char* etag2 = "\"xyz789\"";

    EXPECT_NE(strcmp(etag1, etag2), 0);
}

TEST_F(CacheComparisonTest, LastModifiedExactMatch) {
    const char* lm1 = "Wed, 21 Oct 2015 07:28:00 GMT";
    const char* lm2 = "Wed, 21 Oct 2015 07:28:00 GMT";

    EXPECT_EQ(strcmp(lm1, lm2), 0);
    EXPECT_EQ(strlen(lm1), strlen(lm2));
}

TEST_F(CacheComparisonTest, LastModifiedMismatch) {
    const char* lm1 = "Wed, 21 Oct 2015 07:28:00 GMT";
    const char* lm2 = "Thu, 22 Oct 2015 07:28:00 GMT";

    EXPECT_NE(strcmp(lm1, lm2), 0);
}

// HTTP Date Format Tests
// Format: "DD Mon YYYY HH:MM GMT" (e.g., "21 Oct 2015 07:28 GMT")
class HttpDateFormatTest : public ::testing::Test {};

TEST_F(HttpDateFormatTest, FormatDate) {
    char buffer[100];

    // getHTMLDateFormat(buffer, day, month, year, hour, minute)
    getHTMLDateFormat(buffer, 21, 10, 2015, 7, 28);

    // Should contain GMT
    EXPECT_NE(strstr(buffer, "GMT"), nullptr);

    // Should contain month abbreviation
    EXPECT_NE(strstr(buffer, "Oct"), nullptr);
}

TEST_F(HttpDateFormatTest, FormatDateLength) {
    char buffer[100];
    getHTMLDateFormat(buffer, 1, 1, 2020, 12, 0);

    // Format is "DD Mon YYYY HH:MM GMT" = ~21 chars
    EXPECT_GE(strlen(buffer), 20u);
    EXPECT_LE(strlen(buffer), 25u);
}

TEST_F(HttpDateFormatTest, ContainsYear) {
    char buffer[100];
    getHTMLDateFormat(buffer, 15, 6, 2023, 10, 30);

    // Should contain the year
    EXPECT_NE(strstr(buffer, "2023"), nullptr);
}

TEST_F(HttpDateFormatTest, ContainsMonth) {
    char buffer[100];
    getHTMLDateFormat(buffer, 15, 6, 2023, 10, 30);

    // June should be in the date
    EXPECT_NE(strstr(buffer, "Jun"), nullptr);
}

// Blocked Files Tests
// Note: These require init_file_access_utils() to be called
class BlockedFilesTest : public ::testing::Test {
protected:
    static bool initialized;

    static void SetUpTestSuite() {
        if (!initialized) {
            init_file_access_utils();
            initialized = true;
        }
    }
};

bool BlockedFilesTest::initialized = false;

TEST_F(BlockedFilesTest, UnblockedFile) {
    // Random file should not be blocked
    int blocked = check_blocked_urls("random_file.txt");
    EXPECT_EQ(blocked, 0);
}

TEST_F(BlockedFilesTest, AddAndCheckBlockedFile) {
    WebserverAddBlockedFile("/secret.txt");

    int blocked = check_blocked_urls("secret.txt");
    EXPECT_EQ(blocked, 1);
}

TEST_F(BlockedFilesTest, BlockedFileWithLeadingSlash) {
    WebserverAddBlockedFile("private.html");

    // URL normalization strips leading slash
    int blocked = check_blocked_urls("private.html");
    EXPECT_EQ(blocked, 1);
}

/*
 * The platform's file stream: a request for a registered directory itself
 * (GET /download/) opens the directory, which is not a regular file. Each
 * such request used to keep that descriptor for good.
 */
static int open_descriptors() {
    int n = 0;
    DIR* d = opendir("/proc/self/fd");
    if (d == NULL) {
        return -1;
    }
    while (readdir(d) != NULL) {
        n++;
    }
    closedir(d);
    return n;
}

TEST(PlatformFileStreamTest, DirectoryIsRefusedWithoutKeepingADescriptor) {
    char dir[] = "/tmp/libcwebui_dir_XXXXXX";
    ASSERT_NE(mkdtemp(dir), nullptr);
    int before = open_descriptors();
    ASSERT_GT(before, 0);
    for (int i = 0; i < 100; i++) {
        EXPECT_FALSE(PlatformOpenDataReadStream(dir));
    }
    EXPECT_EQ(open_descriptors(), before);
    rmdir(dir);
}

TEST(PlatformFileStreamTest, CloseAfterAFailedOpenIsHarmless) {
    char dir[] = "/tmp/libcwebui_dir_XXXXXX";
    ASSERT_NE(mkdtemp(dir), nullptr);
    EXPECT_FALSE(PlatformOpenDataReadStream(dir));
    PlatformCloseDataStream();              /* callers that do not check */
    EXPECT_FALSE(PlatformOpenDataReadStream("/tmp/libcwebui_does_not_exist"));
    PlatformCloseDataStream();
    rmdir(dir);
}

TEST(PlatformFileStreamTest, RegularFileStillOpensReadsAndCloses) {
    char path[] = "/tmp/libcwebui_file_XXXXXX";
    int fd = mkstemp(path);
    ASSERT_GE(fd, 0);
    ASSERT_EQ(write(fd, "hello", 5), 5);
    close(fd);
    int before = open_descriptors();
    ASSERT_TRUE(PlatformOpenDataReadStream(path));
    EXPECT_EQ(PlatformGetFileSize(), 5);
    unsigned char buf[8] = {};
    EXPECT_EQ(PlatformReadBytes(buf, 5), 5);
    EXPECT_EQ(memcmp(buf, "hello", 5), 0);
    PlatformCloseDataStream();
    PlatformCloseDataStream();              /* twice: still harmless */
    EXPECT_EQ(open_descriptors(), before);
    unlink(path);
}

/*
 * Resolving a URL in a registered directory: directory and rest of the URL
 * are put together in a 1000 byte buffer. strncat() wrote its terminator
 * one byte past it, and a path cut to fit named another file.
 */
class LocalDirTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        static bool initialised = false;
        if (!initialised) {
            initFileCache();
            init_local_file_system();
            initialised = true;
        }
    }

    /* A directory below /tmp whose path is at least min_length long */
    static std::string make_dir(size_t min_length) {
        char base[] = "/tmp/libcwebui_dir_XXXXXX";
        std::string path = mkdtemp(base);
        while (path.size() < min_length) {
            path += "/" + std::string(100, 'd');
            mkdir(path.c_str(), 0700);
        }
        return path;
    }

    static void write_file(const std::string& path, const char* text) {
        FILE* f = fopen(path.c_str(), "w");
        ASSERT_NE(f, nullptr);
        fputs(text, f);
        fclose(f);
    }
};

TEST_F(LocalDirTest, FileInAShortDirectoryIsFound) {
    std::string dir = make_dir(0);
    write_file(dir + "/hello.txt", "hello");
    add_local_file_system_dir("shortdir", dir.c_str(), 1, 0);
    WebserverFileInfo* f = getFileLocalFileSystem("shortdir/hello.txt");
    ASSERT_NE(f, nullptr);
    EXPECT_EQ(f->DataLenght, 5);
    unlink((dir + "/hello.txt").c_str());
    rmdir(dir.c_str());
}

/* With or without a '/' at the end: exactly one between directory and
 * file. The check used to look at the terminator and always add one. */
TEST_F(LocalDirTest, DirectoryAndFileAreJoinedWithOneSlash) {
    std::string dir = make_dir(0);
    write_file(dir + "/a.txt", "a");
    add_local_file_system_dir("noslash", dir.c_str(), 1, 0);
    add_local_file_system_dir("withslash", (dir + "/").c_str(), 1, 0);

    WebserverFileInfo* f1 = getFileLocalFileSystem("noslash/a.txt");
    WebserverFileInfo* f2 = getFileLocalFileSystem("withslash/a.txt");
    ASSERT_NE(f1, nullptr);
    ASSERT_NE(f2, nullptr);
    EXPECT_EQ(std::string(f1->FilePath), dir + "/a.txt");
    EXPECT_EQ(std::string(f2->FilePath), dir + "/a.txt");
    unlink((dir + "/a.txt").c_str());
    rmdir(dir.c_str());
}

/* Registered without the RAM cache, as WebserverAddFileDirNoCache() does.
 * The flag was left uninitialised - under ASan new blocks are filled with
 * 0xbe, so the old code cached such files after all. */
TEST_F(LocalDirTest, DirectoryWithoutRamCacheIsNotCached) {
    std::string dir = make_dir(0);
    write_file(dir + "/b.txt", "b");
    add_local_file_system_dir("nocache", dir.c_str(), 0, 0);
    WebserverFileInfo* f = getFileLocalFileSystem("nocache/b.txt");
    ASSERT_NE(f, nullptr);
    EXPECT_EQ(f->NoRamCache, 1);
    unlink((dir + "/b.txt").c_str());
    rmdir(dir.c_str());
}

/* An alias that is the start of another one, registered first: the URL of
 * the longer one used to match the shorter alias, and a file of that name
 * in its directory was served instead. */
TEST_F(LocalDirTest, ShorterAliasDoesNotCatchTheLongerOne) {
    std::string pre_dir = make_dir(0);
    std::string prefix_dir = make_dir(0);
    mkdir((pre_dir + "/fix").c_str(), 0700);
    write_file(pre_dir + "/fix/x.txt", "wrong");      /* "pre" + "fix/x.txt" */
    write_file(prefix_dir + "/x.txt", "right!");
    write_file(pre_dir + "/y", "y");

    add_local_file_system_dir("pre", pre_dir.c_str(), 0, 0);      /* first */
    add_local_file_system_dir("prefix", prefix_dir.c_str(), 0, 0);

    WebserverFileInfo* f = getFileLocalFileSystem("prefix/x.txt");
    ASSERT_NE(f, nullptr);
    EXPECT_EQ(std::string(f->FilePath), prefix_dir + "/x.txt");
    EXPECT_EQ(f->DataLenght, 6);

    EXPECT_EQ(getFileLocalFileSystem("prex/y"), nullptr) << "no alias \"prex\"";
    ASSERT_NE(getFileLocalFileSystem("pre/y"), nullptr);

    unlink((pre_dir + "/fix/x.txt").c_str());
    rmdir((pre_dir + "/fix").c_str());
    unlink((pre_dir + "/y").c_str());
    unlink((prefix_dir + "/x.txt").c_str());
    rmdir(pre_dir.c_str());
    rmdir(prefix_dir.c_str());
}

/* Nested aliases whose directories both have the file: the longest alias
 * is tried first, whatever the order of registration. If the file is not
 * in its directory, the shorter one is used, as before. */
TEST_F(LocalDirTest, LongestAliasIsTriedFirstShorterOneStillServesTheRest) {
    std::string outer = make_dir(0);
    std::string inner = make_dir(0);
    mkdir((outer + "/lic").c_str(), 0700);
    write_file(outer + "/lic/a.txt", "outer");
    write_file(outer + "/lic/only_outer.txt", "o");
    write_file(inner + "/a.txt", "inner!");

    add_local_file_system_dir("nest", outer.c_str(), 0, 0);       /* shorter first */
    add_local_file_system_dir("nest/lic", inner.c_str(), 0, 0);

    WebserverFileInfo* f = getFileLocalFileSystem("nest/lic/a.txt");
    ASSERT_NE(f, nullptr);
    EXPECT_EQ(std::string(f->FilePath), inner + "/a.txt") << "longest alias first";

    WebserverFileInfo* g = getFileLocalFileSystem("nest/lic/only_outer.txt");
    ASSERT_NE(g, nullptr);
    EXPECT_EQ(std::string(g->FilePath), outer + "/lic/only_outer.txt") << "falls back to the shorter";

    unlink((outer + "/lic/a.txt").c_str());
    unlink((outer + "/lic/only_outer.txt").c_str());
    rmdir((outer + "/lic").c_str());
    unlink((inner + "/a.txt").c_str());
    rmdir(outer.c_str());
    rmdir(inner.c_str());
}

/* The empty alias is the root, as the web interface registers its pages:
 * it must still match any URL. Registered last here, after the others,
 * so it does not take their URLs in the tests above. */
TEST_F(LocalDirTest, EmptyAliasIsTheRootAndMatchesEveryUrl) {
    std::string root = make_dir(0);
    write_file(root + "/index.html", "<html>");
    add_local_file_system_dir("", root.c_str(), 0, 0);
    WebserverFileInfo* f = getFileLocalFileSystem("index.html");
    ASSERT_NE(f, nullptr);
    EXPECT_EQ(std::string(f->FilePath), root + "/index.html");
    unlink((root + "/index.html").c_str());
    rmdir(root.c_str());
}

TEST_F(LocalDirTest, PathThatDoesNotFitIsNotFound) {
    std::string dir = make_dir(600);
    add_local_file_system_dir("longdir", dir.c_str(), 1, 0);
    std::string url = "longdir/" + std::string(450, 'f');   /* together > 1000 */
    EXPECT_EQ(getFileLocalFileSystem(url.c_str()), nullptr);
    std::string p = dir;
    while (p.size() > 26) {                 /* back up to the mkdtemp() base */
        rmdir(p.c_str());
        p = p.substr(0, p.rfind('/'));
    }
    rmdir(p.c_str());
}
