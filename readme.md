# Lily Extras

This is a personal collection of useful libraries and examples for the Lily programming language.  There is a binary extension `xstr`, and a lexical scanner that uses its string iterator. The lexical scanner is used to parse JSON (with comments ;)) - the json module is a nice example in itself of how to use enums for fun and profit.

## xstr

If Lily has been properly installed, then 'cmake . && make' will give you 'xstr.so' in the current directory.

###define next(s: String, index: Integer): Tuple[String,Integer]

Grab the following character at index. Returns a tuple containing
the character at the index, and the index of the next character.

The returned index is -1 for the last character; thereafter ["",-1]
is returned.

###define size (s: String): Integer

Number of _characters_ in a string

define readall (f: File): String

Read the whole of a file as a string.
Does not check whether the resulting string is valid UTF-8.

###define parse_i (s: String, base: *Integer=10): Option[Integer]

Like String.parse_i, except that you can provide a base (default 10)

define parse_d (s: String): Option[Double]

Convert a string to a Double, returning None if unsuccesful

###define shell (cmd: String): String

Read all the output of a shell command using popen

###define print (values:1...)

Print several values - not restricted to one value like the global
of this name.

###define concat(delim: String, values:1...):String

Concatenate some arbitrary values using the single-character delimiter

###define exit(code: Integer)

Exit the program with the given return code

## scan

Lexical scanners are my favourite text processing tools. They break a string up into _tokens_, 'identifier','number','string','character', etc. This one ignores whitespace and can be taught to ignore comments as well.

Have a look at `test-scan.lly` for usage.

## json

Showing how `enum` naturally expresses recursive data!

## json_parse.lly

An example showing how straightforward it is to parse JSON source into the data format defined by `json.lly`.

This shows more advanced features of `scan`, like skipping line and block comments.

## shell.lly

Showing off some of `xstr`'s capabilities, like Python-style formatting and executing shell commands.

## skelgen.lly

This I present as a non-trivial Lily application. It's still very young, but its purpose is to build a skeleton for implementing a Lily C extension. 

Go into the `os` directory, and run `lily ../skelgen.lly .`. It will read `os.help` and generate `lily_os.c`.  That's it - you fill in the gaps and run `dyna_tools.py` to create the dispatch table.

It can be used also as a quick tool to look at how an individual C function implementing a signature would look like:

```
xstr$ lily skelgen.lly -s '(a: String): String'
 test (a: String): String
void lily_skelgen__test (lily_state *s)
{
    const char* a = lily_arg_string_raw(s,0);
    const char* return_value = "";
    lily_return_string_raw(s,return_value);
}
```