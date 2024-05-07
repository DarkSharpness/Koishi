# Some random thoughts

I hope this could be helpful for those who want to write a compiler from scratch.

First of all, I will list all the references I may have used here. Hope to be useful for you.

- [GCM/GVN](https://dl.acm.org/doi/pdf/10.1145/207110.207154): Global Code Motion, Global Value Numbering
- [SCCP](https://bears.ece.ucsb.edu/class/ece253/papers/wegman91.pdf): Sparse Conditional Constant Propagation
- [ADCE](https://www.cnblogs.com/lixingyang/p/17728846.html): Aggressive dead code elimination.
- [PRE](https://dl.acm.org/doi/pdf/10.1145/319301.319348): Partial redundancy elimination. (I did not implement this) (Another [blog](https://humane-krypton-453.notion.site/SSAPRE-afca1a39cadb46248a529aed371857e5))
- [Magic](https://dl.acm.org/doi/pdf/10.1145/773473.178249): Division by Invariant Integers using Multiplication.
- [SSA-RA](https://compilers.cs.uni-saarland.de/projects/ssara/): SSA-based Register Allocation. (I may write this later)
- [Linear-RA](http://www.christianwimmer.at/Publications/Wimmer10a/Wimmer10a.pdf): Linear Scan Register Allocation on SSA Form. (I implement part of this in my last project)
- [RA](https://zhuanlan.zhihu.com/p/674618277): Just a zhihu article about register allocation.

## AST-level

### Building

Normal visitor mode pattern. With the help of antlr, I don't have to directly parse the input code. With the help of antlr, building the AST tree is too boring that I don't even want to detail it here.

Suggestions for beginners: Just be a bit more careful is enough.

### Checking

Then, it's one of the core part: AST checking. It perform semantic check on the input AST-tree. Based on the nature of Mx, the process is roughly divided into 3 parts: Class-scanning, Function-scanning, Overall-scanning.

#### Class-scanning

In class-scanning, we first create and insert builtin classes to the class_map, which maintains a mapping from a name to class_type pointer. In Mx, there are 6 builtin-types.

```C++
class_map.emplace("int");
class_map.emplace("bool");
class_map.emplace("void");
class_map.emplace("null");
class_map.emplace("string");
class_map.emplace("_Array");
```

Then, we insert the member functions to the scope of the class for builtin types. Why global functions are not inserted, because I made a mistake...... Anyway, this part should be moved to function-scanner, I think.

After that, we check for all the types used in the Mx program. If there's a type which is used in program  (a.k.a: this type has appeared in the class_map, but when scanning all class definition, we don't find the corresponding class), we throw error. In the same stage, we also check whether there are 2 class definitions that declare the same class name.

#### Function-scanning

Function-scanning is a bit more complicated than class-scanning.

First, we create and insert builtin global functions to global-scope (which is currently empty).

```C++
global->insert(create_function(__nil_type, "print", ".print", {{ __str_type, "str" }}));
global->insert(create_function(__nil_type, "println", ".println", {{ __str_type, "str" }}));
global->insert(create_function(__nil_type, "printInt", ".printInt", {{ __int_type, "n" }}));
global->insert(create_function(__nil_type, "printlnInt", ".printlnInt", {{ __int_type, "n" }}));
global->insert(create_function(__str_type, "getString", ".getString"));
global->insert(create_function(__int_type, "getInt", ".getInt"));
global->insert(create_function(__str_type, "toString", ".toString", {{ __int_type, "n" }}));
```

Next, we start to check the functions and classes (yes, class-check again).

For those global functions, we first check that whether the arguments type are void or void array. These are not allowed in Mx. Besides, the return value cann't be a void array. We call this process "check_void". Then, we check the global-scope and class_map to ensure that there's no function or class which shared the same name.

```C++
check_void(__func);
runtime_assert(!class_map.count(__func->name),
    "Function name cannot be class name: \"", __func->name, "\"");
runtime_assert(global->insert(__func),
        "Duplicated function name: \"", __func->name, "\"");
```

For member functions, the procedures are similar. First, "check_void". Then, check the class_scope to ensure that there's no member function or member variables sharing the same name. Specially, the constructor's name must be identical with that of the class it belongs. Otherwise, it's invalid. (Specially, I insert "this" pointer to the member function's scope by the way).

As for member variables, we just need to check its conflict with other member variables/functions. Also, the initializer of it should be null (But, this can be supported in future as a feature).

#### Overall-scanning

Just like building an AST-tree, we visit the AST-tree, using visitor mode pattern.

First, we created a special function, called "_Global_init". It is used to help initialize all global variables. When we visit global-variable-definition (which will be introduce in the following paragraph) with a non-constant initializer, we will move the initialization to  "_Global_init" functions.

Next, we started to visit the AST-tree. This process is boring as well, but requires much more patience and details. For example, which scope to look up when meeting variable defines? For simplicity, I add a pointer to scope for all AST node. This is surely not the best solution, but a really simple-to-implement one.

Next example: type system. Is the expression is assignable? Is the identifier a function or a variable? How or where to store these meta data? I solve it in the design of AST type system.

```C++
struct typeinfo {
    class_type *  base  {}; // Type of expression.
    int     dimensions  {}; // Dimensions of array.
    bool    assignable  {}; // Whether left value.
    /* Return the name of the typeinfo. */
    std::string data() const noexcept;
    friend bool operator ==
        (const typeinfo &__lhs, const typeinfo &__rhs) noexcept {
        return __lhs.base == __rhs.base &&
            __lhs.dimensions == __rhs.dimensions;
    }
    bool is_function() const noexcept { return base && base->name.empty(); }
};
```

For function types, its base pointer point to a special class, whose name is empty. For left-value types (e.g. all variables, prefix ++/-- expression, array-access result), its assignable state is set to true.

After that, the type check goes easy: First visit down the expression. Then look up its typeinfo. If current expression requires the subexpression to be assignable, but it does not, throw error!

Finally, we should do some corner checks. Whether the main function is a valid one ( int main() {...} ). Whether the "_Global_init" functions is used (if not, remove it; otherwise, call it at the beginning of main function).

### Optimization

There isn't much to optimize in AST-level for me. The only AST-level optimization for me currently is constant folding. What does it means? Fold constant expressions, like `114 + 514 / 19 - 19 * 8 % 10`. I know that's not strong enough, but that's really easy to implement, and doesn't requires any data-flow analysis. I just write that by the way.

## IR-level

TODO:
