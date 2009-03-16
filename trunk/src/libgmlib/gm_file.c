#include "gm_file.h"

gchar *gm_tempname(gchar * path, const gchar * name_template)
{
    gchar *result;
    gchar *replace;
    gchar *basename;
    gchar *localpath;

    basename = g_strdup(name_template);

    if (path == NULL && g_getenv("TMPDIR") == NULL) {
        localpath = g_strdup("/tmp");
    } else if (path == NULL && g_getenv("TMPDIR") != NULL) {
        localpath = g_strdup(g_getenv("TMPDIR"));
    } else {
        localpath = g_strdup(path);
    }

    while ((replace = g_strrstr(basename, "X"))) {
        replace[0] = (gchar) g_random_int_range((gint) 'a', (gint) 'z');
    }

    result = g_strdup_printf("%s/%s", localpath, basename);
    g_free(basename);
    g_free(localpath);

    return result;
}
