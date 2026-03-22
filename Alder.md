# Alder Language Reference

Alder is a statically typed language that compiles to C++. The syntax is Python-style (indentation-based blocks), but types are explicit and required.

---

## Compilation

```
alder <source.adr>                   # compile to binary (named after the source file)
alder <source.adr> -o <binary>       # compile to a specific binary name
alder <source.adr> --emit-cpp <file> # write C++ output only, skip compilation
```

The compiler transpiles Alder to C++ and then invokes `clang++` (or `g++` as a fallback) automatically. No manual C++ step required.

```
alder factorial.adr          # produces ./factorial
alder factorial.adr -o calc  # produces ./calc
```

Requires `clang++` or `g++` to be on your PATH.

---

## Comments

```
# this is a comment
```

No multi-line comments.

---

## Types

| Alder      | C++ equivalent    |
|------------|-------------------|
| `int`      | `int`             |
| `float`    | `double`          |
| `string`   | `std::string`     |
| `bool`     | `bool`            |
| `void`     | `void`            |
| `list[T]`  | `std::vector<T>`  |

---

## Variables

```
name: Type = value
```

The type comes after the colon, the value after `=`. Both are required — there's no uninitialised declaration.

```
x: int = 10
pi: float = 3.14
greeting: string = "hello"
flag: bool = true
nums: list[int] = [1, 2, 3]
```

### Immutable variables

Use `final` to make a variable constant. It must still be initialised at declaration.

```
final MAX: int = 100
final NAME: string = "alder"
```

---

## Literals

```
42          # int
3.14        # float
"hello"     # string (double quotes only)
true        # bool
false       # bool
[1, 2, 3]  # list literal
```

---

## Operators

### Arithmetic

```
x + y
x - y
x * y
x / y
```

### Comparison

```
x == y
x != y
x < y
x <= y
x > y
x >= y
```

### Logical

```
x and y
x or y
not x
```

### Bitwise / Shifts

```
x << y    # left shift
x >> y    # arithmetic right shift
x >>> y   # logical right shift
```

### Assignment

```
x = expr
a[i] = expr
```

Assignment is an expression, so it returns the assigned value. The left side can be a variable or an index expression.

### Precedence (high to low)

| Level | Operators                |
|-------|--------------------------|
| 9     | function call, indexing  |
| 8     | `not`, unary `-`         |
| 7     | `*`, `/`                 |
| 6     | `+`, `-`                 |
| 5.5   | `<<`, `>>`, `>>>`        |
| 5     | `<`, `<=`, `>`, `>=`     |
| 4     | `==`, `!=`               |
| 3     | `and`                    |
| 2     | `or`                     |
| 1     | `=`                      |

---

## Builtins

### I/O

```
print("hello")         # prints to stdout with a trailing newline
print("x =", x)        # multiple args, no separator between them
print()                # blank line

name: string = input()           # read a line from stdin
name: string = input("name: ")   # with a prompt
```

### Lists

```
append(nums, 42)       # add element to the end
pop(nums)              # remove last element
n: int = len(nums)     # number of elements
```

### Range

Returns a `list[int]`. Useful with `for` loops.

```
for (i: int) in range(5):        # 0, 1, 2, 3, 4
    print(i)

for (i: int) in range(2, 5):     # 2, 3, 4
    print(i)
```

### Type conversion

```
s: string = str(42)        # number to string
n: int    = int(3.9)       # float to int (truncates)
f: float  = float(10)      # int to float
```

### Math

All math functions require numeric arguments.

```
abs(x)           # absolute value
min(a, b)        # smaller of two values
max(a, b)        # larger of two values
pow(base, exp)   # exponentiation
sqrt(x)          # square root
floor(x)         # round down
ceil(x)          # round up
```

---

## Functions

```
fn name(param1: Type1, param2: Type2) -> ReturnType:
    # body
```

Parameters use the same `name: Type` form as variables. The return type follows `->`. Functions without a meaningful return value use `void`.

```
fn add(x: int, y: int) -> int:
    return x + y

fn greet(name: string) -> void:
    print(name)
```

Functions must be declared before use. There are no lambdas or closures.

---

## Control Flow

### if / elif / else

```
if condition:
    # ...
elif other_condition:
    # ...
else:
    # ...
```

`elif` and `else` are optional. Conditions are plain expressions — no parentheses needed (though they work).

```
if x > 0:
    result: string = "positive"
elif x == 0:
    result: string = "zero"
else:
    result: string = "negative"
```

### while

```
while condition:
    # body
```

```
i: int = 0
while i < 10:
    i = i + 1
```

No `break` or `continue` yet.

### for

Iterates over a collection. The binding declares a typed loop variable.

```
for (var: Type) in iterable:
    # body
```

```
nums: list[int] = [1, 2, 3, 4, 5]
for (n: int) in nums:
    print(n)
```

Multiple bindings (for destructuring tuples):

```
for (i: int, v: int) in pairs:
    print(i)
```

---

## Indexing

```
nums: list[int] = [10, 20, 30]
first: int = nums[0]
nums[1] = 99
```

## Slicing

Returns a new list containing the selected range. Both bounds are optional.

```
nums: list[int] = [10, 20, 30, 40, 50]

a: list[int] = nums[1:3]    # [20, 30]
b: list[int] = nums[:2]     # [10, 20]       — from start
c: list[int] = nums[2:]     # [30, 40, 50]   — to end
```

Negative indices count from the end:

```
tail: list[int] = nums[-2:]  # [40, 50]
```

Slices never mutate the original — they return a copy.

---

## Return

```
return expr
```

Every non-void function needs a `return`. Falling off the end of a function without returning is undefined behaviour (the C++ compiler will catch it).

---

## Indentation

Blocks are delimited by indentation. The indentation level must be consistent within a block — mixing tabs and spaces will cause a parse error. Each nested level must increase the column beyond the enclosing block.

```
fn example() -> int:
    x: int = 1          # 4-space indent
    if x > 0:
        x = x + 1      # 8-space indent
    return x
```

---

## Full example

```
fn factorial(n: int) -> int:
    result: int = 1
    i: int = 1
    while i <= n:
        result = result * i
        i = i + 1
    return result

fn main() -> int:
    x: int = factorial(5)
    return 0
```

Transpiles to:

```cpp
int factorial(int n) {
    int result = 1;
    int i = 1;
    while ((i <= n)) {
        result = (result * i);
        i = (i + 1);
    }
    return result;
}

int main() {
    int x = factorial(5);
    return 0;
}
```
