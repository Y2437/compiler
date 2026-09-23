const int gca = 1, gcb = 2, gcc = 3;
// 测试常量声明中一条语句声明多个常量的情况
const int gcsingle = 10;
// 测试常量声明中只声明一个常量的情况
int gva = 5, gvb, gvc = gca + gcb;
// 测试变量声明中一条语句声明多个变量的情况，以及全局变量使用常量表达式初始化的情况
int gvsingle;
// 测试全局变量声明时不给出初始值的情况

int add(int x, int y) {
  // 测试带有多个形参的函数定义的情况
  return x + y;
}

void do_nothing() {
  // 测试无形参的函数定义以及空语句块的情况
}

int accumulate() {
  static int cnt = 1, extra;
  // 测试静态局部变量声明的情况，静态变量只初始化一次
  cnt = cnt + 1;
  extra = extra + 2;
  return cnt + extra;
}

int main() {
  printf("24373169\n");
  // 测试printf语句只包含字符串常量的情况，此处输出学号
  int in1 = get_int();
  // 测试get_int函数读取十进制整数输入的情况
  int in2 = get_int();
  printf("input values: %d %d\n", in1, in2);
  int a = 3, b = 4, c = 5;
  // 测试函数内一条变量声明语句声明多个变量的情况
  const int ca = 7, cb = 8, cc = 9;
  // 测试函数内常量声明的情况
  printf("local const: %d %d %d\n", ca, cb, cc);
  int r1 = a + b - c;
  // 测试加减表达式的情况
  printf("add sub: %d\n", r1);
  int r2 = a * b / c % 2;
  // 测试乘除模表达式的情况
  printf("mul div mod: %d\n", r2);
  int r3 = 0;
  if (a < b) {
    // 测试小于关系运算的情况
    r3 = 1;
  }
  printf("less than: %d\n", r3);
  int r4 = 0;
  if (a > b) {
    // 测试大于关系运算的情况
    r4 = 1;
  }
  printf("greater than: %d\n", r4);
  int r5 = 0;
  if (a <= b) {
    // 测试小于等于关系运算的情况
    r5 = 1;
  }
  printf("less equal: %d\n", r5);
  int r6 = 0;
  if (a >= b) {
    // 测试大于等于关系运算的情况
    r6 = 1;
  }
  printf("greater equal: %d\n", r6);
  int r7 = 0;
  if (a == b) {
    // 测试相等性判断的情况
    r7 = 1;
  }
  printf("equal: %d\n", r7);
  int r8 = 0;
  if (a != b) {
    // 测试不相等判断的情况
    r8 = 1;
  }
  printf("not equal: %d\n", r8);
  int r9 = -a;
  // 测试单目取负运算符的情况
  printf("unary minus: %d\n", r9);
  int r10 = +b;
  // 测试单目取正运算符的情况
  printf("unary plus: %d\n", r10);
  int r11 = ((a + b) * (c - 1));
  // 测试嵌套括号表达式的情况
  printf("nested paren: %d\n", r11);
  int rin = in1 * in2 + a;
  // 测试输入数据参与乘法与加法混合运算的情况
  printf("input mul add: %d\n", rin);
  a = b + c;
  // 测试赋值语句的情况
  printf("assign: %d\n", a);
  a + b;
  // 测试表达式作为语句且结果被丢弃的情况
  ;
  // 测试空表达式语句的情况
  add(a, b);
  // 测试函数调用语句且返回值被丢弃的情况
  printf("func add: %d\n", add(a, b));
  do_nothing();
  // 测试调用无形参函数的情况
  {
    int a = 100;
    // 测试内层语句块中定义与外层同名变量并隐藏外层变量的情况
    printf("shadow var: %d\n", a);
    // 测试printf语句带表达式参数的情况
  }
  if (r1 > r2) {
    // 测试带有else分支的if语句的情况
    r1 = r1 - 1;
  } else {
    r1 = r1 + 1;
  }
  printf("if else result: %d\n", r1);
  if (r3 <= r4)
    // 测试不带else分支的if语句的情况
    r3 = r3 + 1;
  printf("if no else: %d\n", r3);
  int i = 0;
  int total = 0;
  while (i < 10) {
    // 测试while循环语句的情况
    i = i + 1;
    if (i % 2 == 0) {
      continue;
      // 测试continue语句跳过本次循环的情况
    }
    if (i > 7) {
      break;
      // 测试break语句跳出循环的情况
    }
    total = total + i;
  }
  printf("while break continue: %d %d\n", i, total);
  int k = 0;
  while (k < 2)
    // 测试循环体为单条语句的while语句的情况
    k = k + 1;
  printf("single stmt while: %d\n", k);
  if (!(a - b)) {
    // 测试单目逻辑非运算符作用于括号内算术表达式的情况
    a = a + 1;
  }
  printf("logical not: %d\n", a);
  int first = accumulate();
  int second = accumulate();
  // 测试静态局部变量在多次调用之间保留状态的情况
  printf("static keep: %d %d\n", first, second);
  printf("global vars: %d %d\n", gva, gvc);
  printf("input final: %d\n", rin + in1 - in2);
  // 测试输入数据经过多次运算后的输出结果的情况
  printf("global const more: %d %d\n", gcc, gcsingle);
  // 测试读取全局常量的值的情况
  printf("global zero init: %d %d\n", gvb, gvsingle);
  // 测试全局变量不给出初始值而被置为零的情况
  return 0;
}
