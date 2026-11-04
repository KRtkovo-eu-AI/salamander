// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-04 | Test portable overwrite and rename decisions. | Those decisions must hold without a phone attached. | | Grok 4.7 |

#include "../plugins/portables/wpdnamepolicy.h"

#include <cstdio>
#include <cstring>

static int g_failures = 0;

static void Expect(bool condition, const char* message)
{
    if (!condition)
    {
        std::fputs(message, stderr);
        std::fputc('\n', stderr);
        ++g_failures;
    }
}

static int ICmp(const char* left, const char* right)
{
    return _stricmp(left, right);
}

int main()
{
    Expect(WpdChildMatchesEitherName("Photo.jpg", true, "album", true, "photo.jpg", ICmp),
           "original file name matches regardless of case");
    Expect(WpdChildMatchesEitherName("IMG_1", true, "Photo.jpg", true, "photo.JPG", ICmp),
           "object name matches when the original file name does not");
    Expect(!WpdChildMatchesEitherName("Photo.jpg", true, "album", true, "other.jpg", ICmp),
           "neither stored name matches");
    Expect(!WpdChildMatchesEitherName("", false, "", false, "photo.jpg", ICmp),
           "missing names do not match");

    Expect(!WpdMayCreateWhenNamePresent(true),
           "create is refused while the name is still present");
    Expect(WpdMayCreateWhenNamePresent(false),
           "create is allowed once the name is gone");

    Expect(WpdRenamePlanForObject(true) == WpdRenameFolderNameOnly,
           "folders set WPD_OBJECT_NAME only");
    Expect(WpdRenamePlanForObject(false) == WpdRenameFileNameAndOriginalSeparately,
           "files set both names in separate updates");
    Expect(WpdRenamePlanForObject(true) != WpdRenamePlanForObject(false),
           "folder and file rename plans stay distinct");

    return g_failures == 0 ? 0 : 1;
}
