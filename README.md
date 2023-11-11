Example Tar writer
==================

Example C code to create a Tar archive file with several files included.

> FYI: [Basic Tar Format](https://www.gnu.org/software/tar/manual/html_node/Standard.html)

Usage
-----

```console
$ git@github.com:STARRY-S/tar-example-c.git && cd tar-example-c
$ gcc ./main.c && ./a.out
$ tar --list --file ./example.tar
1.txt
test/
test/2.txt
```

License
-------

[MIT](LICENSE)