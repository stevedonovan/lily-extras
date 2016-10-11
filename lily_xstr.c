#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lily_api_embed.h"
#include "lily_api_alloc.h"
#include "lily_api_value.h"
#include "lily_api_msgbuf.h"

/**
package xstr

Provides a few extra operations on strings; an iterator, the size of a string
in characters, and a means to read a whole file.
*/

// this is private to be core, so let's define it again ;)
static const char move_table[256] =
{
     /* 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
/* 0 */ 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 1 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 2 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 3 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 4 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 5 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 6 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 7 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 8 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 9 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* A */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* B */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* C */ 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
/* D */ 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
/* E */ 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
/* F */ 4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/**
define next(s: String, index: Integer): Tuple[String,Integer]

Grab the following character at index. Returns a tuple containing
the character at the index, and the index of the next character.

The returned index is -1 for the last character; thereafter ["",-1]
is returned.
*/
void lily_xstr__next(lily_state *s)
{
    const char *input = lily_arg_string_raw(s,0);
    int index  = lily_arg_integer(s,1);

    // how many chars in this character?
    int to_copy = index > -1 ? move_table[(uint8_t)input[index]] : 0;

    // make a copy!
    lily_string_val *string_result = lily_new_raw_string_sized(input + index, to_copy);

    // and let's move onto the next....
    index += to_copy;
    if (index > -1 && input[index] == 0) { // ALWAYS nul-terminated...
        index = -1;
    }

    // pack the character and the next index into a tuple
    lily_list_val *lv = lily_new_list_val_n(2);
    lily_list_set_string(lv,0,string_result);
    lily_list_set_integer(lv,1,index);
    lily_return_tuple(s,lv);
}

/**
define size (s: String): Integer

Number of _characters_ in a string
*/
void lily_xstr__size(lily_state *s)
{
    char *str = lily_arg_string_raw(s,0);
    int res = 0;
    while (*str) {
        ++res;
        str += move_table[(uint8_t)*str];
    }
    lily_return_integer(s, res);
}

static lily_string_val *fill_string_from_file(FILE *f, int drop_lf)
{
    size_t  bufsize = 64;
    char *buffer = lily_malloc(bufsize);
    int pos = 0, nread;
    int nbuf = bufsize/2;
    while (1) {
        nread = fread(buffer+pos,1,nbuf,f);
        pos += nread;
        if (nread < nbuf) {
            if (drop_lf && nread > 0)
               --pos;
            buffer[pos] = '\0';
            break;
        }
        if (pos >= bufsize) {
           nbuf = bufsize;
           bufsize *= 2;
           buffer = lily_realloc(buffer,bufsize);
        }
    }
    lily_string_val *sv = lily_new_raw_string_sized(buffer,pos);
    lily_free(buffer);
    return sv;
}


/**
define readall (f: File): String

Read the whole of a file as a string.
Does not check whether the resulting string is valid UTF-8.
*/
void lily_xstr__readall(lily_state *s)
{
    lily_file_val *filev = lily_arg_file(s,0);
    lily_file_ensure_readable(s,filev);
    lily_return_string(s, fill_string_from_file(lily_file_get_raw(filev),0));
}

/**
define parse_i (s: String, base: *Integer=10): Option[Integer]

Like String.parse_i, except that you can provide a base (default 10)
*/
void lily_xstr__parse_i (lily_state *s)
{
    const char* s_ = lily_arg_string_raw(s,0);
    int64_t base = (lily_arg_count(s) > 1) ? lily_arg_integer(s,1) : 10;
    char *endp;
    int64_t return_some = strtoll(s_,&endp, base);

    if (*s_ == '\0' ||  *endp != '\0') {
        lily_return_empty_variant(s, lily_get_none(s));
        return;
    }
    
    lily_instance_val *some_val = lily_new_some();
    lily_variant_set_integer(some_val, 0, return_some);
    lily_return_filled_variant(s,some_val);   
}

/**
define parse_d (s: String): Option[Double]

Convert a string to a Double, returning None if unsuccesful
*/
void lily_xstr__parse_d (lily_state *s)
{
    const char* s_ = lily_arg_string_raw(s,0);
    char *endp;
    double return_some = strtod(s_,&endp);

    if (*s_ == '\0' ||  *endp != '\0') {
        lily_return_empty_variant(s, lily_get_none(s));
        return;
    }
    
    lily_instance_val *some_val = lily_new_some();
    lily_variant_set_double(some_val, 0, return_some);
    lily_return_filled_variant(s,some_val);   
}

/**
define shell (cmd: String): String

Read all the output of a shell command using popen
*/
void lily_xstr__shell(lily_state *s)
{
    const char *cmd = lily_arg_string_raw(s,0);
    FILE *f = popen(cmd,"r");
    lily_return_string(s, fill_string_from_file(f,1));
    pclose(f);
}

static void concat(lily_state *s, lily_msgbuf *msgbuf, char ch, lily_list_val *lv)
{
   int i;
   lily_mb_flush(msgbuf);
   for (i = 0; i < lily_list_num_values(lv); i++) {
      lily_value *v = lily_list_value(lv,i);
      if (i > 0)
         lily_mb_add_char(msgbuf,ch);
      lily_mb_add_value(msgbuf, s, v);
   }
}

/**
define print (values:1...)

Print several values
*/
void lily_xstr__print(lily_state *s)
{
    lily_list_val *lv = lily_arg_list(s,0);

    lily_msgbuf *msgbuf = lily_get_msgbuf(s);
    concat(s,msgbuf,'\t',lv);
    lily_mb_add_char(msgbuf,'\n');
    fputs(lily_mb_get(msgbuf),stdout);
    lily_mb_flush(msgbuf);
    lily_return_unit(s);
}

/**
define concat(delim: String, values:1...):String

Concatenate some arbitrary values using the single-character delimiter
*/
void lily_xstr__concat(lily_state *s)
{
    const char *delim = lily_arg_string_raw(s,0);
    lily_list_val *lv = lily_arg_list(s,1);

    lily_msgbuf *msgbuf = lily_get_msgbuf(s);
    concat(s,msgbuf,delim[0],lv);
    lily_return_string(s,lily_new_raw_string(lily_mb_get(msgbuf)));
    lily_mb_flush(msgbuf);
}

static int char_index(const char *s, int idx, char ch)
{
    const char *P = strchr(s+idx,ch);
    if (! P)
        return -1;
    else
        return (int)((uintptr_t)P - (uintptr_t)s);
}

// note the assumption ;)
static int convert_i(char d)
{
    return (int)d - (int)'0';
}

/**
define format(fmt: String, values:1...):String

A simplified Python-style formatter
*/
void lily_xstr__format(lily_state *s)
{
    const char *fmt = lily_arg_string_raw(s,0);
    lily_list_val *lv = lily_arg_list(s,1);

    int lsize = lily_list_num_values(lv);
    lily_msgbuf *msgbuf = lily_get_msgbuf(s);
    lily_mb_flush(msgbuf);

    int idx = 0, last_idx = 0, i_def = 0;
    while (last_idx != -1) {
        idx = char_index(fmt,last_idx,'{');
        if (idx > -1) {
            if (idx > last_idx)
                lily_mb_add_range(msgbuf,fmt,last_idx, idx); // just before {
            // next value
            int i = i_def;
            idx++; // skip '{'
            if (isdigit(fmt[idx])) {
                i = convert_i(fmt[idx]);
                idx++;
            } else {
                i = i_def++;
            }
            if (i >= lsize) {
                lily_error(s, SYM_CLASS_INDEXERROR, "too many format items");
            }
            idx++; // skip '}'
            lily_value *v = lily_list_value(lv,i);
            lily_mb_add_value(msgbuf, s, v);
        }  else { // end of format string
            lily_mb_add_range(msgbuf,fmt,last_idx, strlen(fmt));
        }
        last_idx = idx;
    }
    lily_return_string(s,lily_new_raw_string(lily_mb_get(msgbuf)));
    lily_mb_flush(msgbuf);
}

/**
define exit(code: Integer)

Exit the program with the given return code
*/
void lily_xstr__exit(lily_state *s)
{
    int64_t code = lily_arg_integer(s,0);
    exit((int)code);
    
}

#include "dyna_xstr.h"
