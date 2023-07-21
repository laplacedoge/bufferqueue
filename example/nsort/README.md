# nsort

## Usage

```plain
nsort [-a] [-d] num0 num1 num2 ... numN
Sorts the given numbers.
Options:
  -a  Sort in ascending order (default)
  -d  Sort in descending order"
```
## Example

Sorting numbers in ascending order:

```shell
$ ./nsort -a 1 7 15 3 4 14 11 8 9 16 12 2 10 5 13 0 6
Sorted numbers in ascending order:
0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16
```

Sorting numbers in descending order:

```shell
$ ./nsort -d 1 7 15 3 4 14 11 8 9 16 12 2 10 5 13 0 6
Sorted numbers in descending order:
16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
```
