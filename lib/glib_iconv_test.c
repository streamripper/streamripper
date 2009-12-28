#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>

void
fopen_test (void)
{
    FILE* fp;
    
#if defined (commentout)
    fp = fopen ("testme.txt", "w");
    if (!fp) {
	printf ("fopen failed.\n");
    } else {
        fclose (fp);
    }
    fp = _wfopen (L"testme.txt", L"w");
    if (!fp) {
	printf ("_wfopen failed.\n");
    } else {
        fclose (fp);
    }
#endif
    fp = g_fopen ("testme.txt", "w");
    if (!fp) {
	printf ("g_fopen failed.\n");
    } else {
        fclose (fp);
    }
}

void
run_codeset_test (char* from_codeset, char* to_codeset) {
    GIConv giconv;
    GError *error = 0;
    gchar* out;

    printf ("Converting %s -> %s\n", from_codeset, to_codeset);
    giconv = g_iconv_open (to_codeset, from_codeset);
    if (giconv == (GIConv) -1) {
	printf ("g_iconv_open error\n");
	return;
    }

    out = g_convert_with_iconv ("A", 1, giconv, 0, 0, &error);

    if (error && error->code == G_CONVERT_ERROR_ILLEGAL_SEQUENCE) {
	printf ("Illegal sequence\n");
    } else {
	printf ("Successful conversion: %s\n", out);
    }
    g_iconv_close (giconv);
}

void
codeset_test (void) 
{
    run_codeset_test ("CP1252", "UTF-8");
    run_codeset_test ("UTF-8", "CP1252");
    run_codeset_test ("UTF-8", "US-ASCII");
}

/* Convert a string, replacing unconvertable characters.
   Returns a gchar* string, which must be freed by caller using g_free. */
gchar*
convert_string (char* instring, gsize len, 
		char* from_codeset, char* to_codeset)
{
    GIConv giconv;
    GError *error = 0;
    gchar* os;
    gsize br, bw;

    giconv = g_iconv_open (to_codeset, from_codeset);
    if (giconv == (GIConv)-1) {
	printf ("g_iconv_open error\n");
	return 0;
    }

    os = g_convert_with_iconv (instring, len, 
			       giconv, &br, &bw, &error);

    if (error) {
	switch (error->code) {
	case G_CONVERT_ERROR_ILLEGAL_SEQUENCE:
	    printf ("Illegal sequence\n");
	    return 0;
	case G_CONVERT_ERROR_FAILED:
	case G_CONVERT_ERROR_PARTIAL_INPUT:
	case G_CONVERT_ERROR_NO_CONVERSION:
	default:
	    printf ("other error\n");
	    return 0;
	}
    }
    g_iconv_close (giconv);
    return os;
}

#if (0)
void
test_library (void)
    hlibiconv = LoadLibrary(dllname);
01085         if (hlibiconv != NULL)
01086         {
01087             if (hlibiconv == hwiniconv)
01088             {
01089                 FreeLibrary(hlibiconv);
01090                 hlibiconv = NULL;
01091                 continue;
01092             }
01093             break;
01094         }
#endif

void
g_iconv_test () {
    char instring[] = { 'A', '\0' };
    gchar* os;
    char *to_codeset, *from_codeset;

    // "ISO-8859-15", "US-ASCII"
    to_codeset = "US-ASCII";
    to_codeset = "UTF-8";
    to_codeset = "CP1252";
    from_codeset = "CP1252";
    from_codeset = "UTF-8";

    os = convert_string (instring, strlen(instring), 
			 from_codeset, to_codeset);
    if (os) {
	printf ("os = %s\n", os);
	g_free (os);
    }
}

int main (int argc, char* argv[])
{
    printf ("Hello world\n");
    fopen_test ();
//    codeset_test ();
    return 0;
}

