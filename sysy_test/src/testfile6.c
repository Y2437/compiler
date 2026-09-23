const int gca = 1, gcb = 2, gcn = 3;
// 测试常量声明中一条语句声明多个常量的情况
const char gcx = 'x', gcy = 'y', gcz = 'z';
// 测试字符类型常量声明的情况
const int gclen = 4;
const int gcexp = 65 + 1;
// 测试常量表达式在编译期求值的情况
const int gcarr[3] = {4, 5, 6};
// 测试常量数组带有完整初值列表的情况
const int gcarr2[3] = {};
// 测试常量数组初值列表为空的情况
const int gcarr3[3] = {7};
// 测试常量数组初值列表只给出部分元素的情况
const char gccarr[3] = {'p', 'q', 'r'};
// 测试字符常量数组使用字符常量列表初始化的情况
const char gcstr[6] = "hello";
// 测试字符常量数组使用字符串常量初始化的情况
int gva = 1, gvb, gvc = gca + gcb;
// 测试变量声明中部分变量带初始值且部分变量不带初始值的情况
char gda = 'd', gdb, gdc = 'f';
int gia[3] = {1, 2, 3};
int gib[3];
int gib3[3] = {4};
char gsa[6] = "hello";
char gsb[3] = {'a', 'b', 'c'};
int side = 0;

int side_effect() {
  // 测试带有副作用的函数用于检验短路求值规则的情况
  side = side + 1;
  return 1;
}

int one(int q) {
  // 测试带有一个形参的函数定义的情况
  return q;
}

int counter() {
  static int cnt = 0, uncnt;
  // 测试静态局部变量只初始化一次并在多次调用之间保留状态的情况
  cnt = cnt + 1;
  uncnt = uncnt + 2;
  return cnt + uncnt;
}

void void_empty() {
  // 测试函数体为空的void类型函数的情况
}

void void_params(int v1, char v2, int v3[], int v4) {
  // 测试形参表中普通整型形参、字符形参与数组形参混合出现的情况
  v3[0] = v1 + v4;
  v2 = v2 + 'b';
  // 测试字符类型变量与字符常量进行加法运算的情况
  return;
}

int int_params(int p1, char p2, int p3[]) {
  p3[1] = p1;
  p2 = p2 + 'c';
  return p1 + p3[0];
}

char char_func(char c1, char c2) {
  // 测试返回类型为char的函数的情况
  return c1 + c2;
}

char first_char(char s[]) {
  return s[0];
}

int main() {
  printf("24373169\n");
  // 测试printf语句只包含字符串常量的情况，此处输出学号
  int in1 = get_int();
  // 测试get_int函数读取整型输入的情况
  char inc = get_char();
  // 测试get_char函数读取字符输入的情况
  char instr[16];
  get_string(instr, 16);
  // 测试get_string函数读取字符串输入到字符数组的情况
  printf("get int: %d\n", in1);
  printf("get char: %c\n", inc);
  printf("get string: %s", instr);
  int a = 1, b = 2, c = 3;
  char x = 'a', y = 'b', z = 'c';
  int arr[4] = {4, 5, 6, 7};
  int arr2[3] = {8};
  printf("part init: %d %d %d\n", arr2[0], arr2[1], arr2[2]);
  int arr3[2] = {};
  int arr4[2];
  char cs[8] = "sysy\n";
  char cs2[4] = "ab";
  // 测试数组长度大于字符串长度时剩余元素被置为零的情况
  printf("string pad: %d\n", (int)cs2[2]);
  char cs3[3] = {'m', 'n', 'o'};
  const int care[3] = {10, 20, 30};
  // 测试常量数组使用整型常量列表初始化的情况
  const char cstr[5] = "test";
  int convarr[100];
  // 测试使用整型常量作为数组长度的情况
  convarr[0] = (int)'d';
  printf("cast length array: %d\n", convarr[0]);
  printf("const string: %s\n", cstr);
  printf("char list init: %c %c\n", cs3[0], cs3[2]);
  printf("empty list init: %d\n", arr3[0]);
  static int st1 = 11, st2, st3 = 13;
  // 测试函数内静态局部变量声明的情况
  printf("static init: %d %d %d\n", st1, st2, st3);
  int r1 = a + b - c;
  printf("add sub: %d\n", r1);
  int r2 = a * b / c % 5;
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
  int r9 = 0;
  if (a < b && b < c) {
    // 测试逻辑与运算组成条件表达式的情况
    r9 = 1;
  }
  printf("land value: %d\n", r9);
  int r10 = 0;
  if (a > b || b < c) {
    // 测试逻辑或运算组成条件表达式的情况
    r10 = 1;
  }
  printf("lor value: %d\n", r10);
  int r11 = -a;
  printf("unary minus: %d\n", r11);
  int r12 = +b;
  printf("unary plus: %d\n", r12);
  int r13 = ((a + b) * (c - 1));
  printf("nested paren: %d\n", r13);
  int r14 = (int)x;
  // 测试将字符类型显式转换为整型的情况
  printf("char to int: %d\n", r14);
  char r15 = (char)((int)x + 1);
  // 测试整型运算结果显式转换回字符类型的情况
  printf("int to char: %c\n", r15);
  int r17 = (int)x + (int)y;
  printf("cast add: %d\n", r17);
  char r18 = (char)(r17 - 194);
  printf("cast back: %d\n", (int)r18);
  int r19 = gcarr[1];
  // 测试读取常量数组元素的情况
  printf("const array elem: %d\n", r19);
  int r20 = care[2];
  printf("local const array: %d\n", r20);
  a = b + c;
  a = a + in1;
  // 测试输入数据参与赋值运算的情况
  printf("input assign: %d\n", a);
  arr[0] = a + arr[0];
  printf("array assign: %d\n", arr[0]);
  cs[0] = 'k';
  arr4[0] = 1;
  arr4[1] = arr4[0] + 2;
  printf("array chain: %d %d\n", arr4[0], arr4[1]);
  a + b;
  // 测试表达式作为语句且结果被丢弃的情况
  printf("int params ret: %d\n", int_params(a, x, arr));
  // 测试函数调用中数组作为实参的情况
  printf("array modified: %d\n", arr[1]);
  void_params(a, x, arr, b);
  printf("void params: %d\n", arr[0]);
  printf("one elem param: %d\n", one(arr[0]));
  // 测试数组元素作为函数实参的情况
  z = char_func(inc, '\0');
  // 测试输入字符作为函数实参的情况
  printf("char func: %c\n", z);
  z = first_char(cs);
  printf("first char: %c\n", z);
  side_effect();
  printf("side effect: %d\n", side);
  void_empty();
  ;
  // 测试空表达式语句的情况
  {
    int inner = 5;
    // 测试语句块内声明局部变量的情况
    a = a + inner;
    printf("block inner: %d\n", a);
  }
  r1 = arr[c];
  // 测试使用变量作为数组下标的情况
  printf("var index: %d\n", r1);
  cs[c] = 'q';
  if (r1 > r2) {
    r1 = r1 - 1;
  } else {
    r1 = r1 + 1;
  }
  printf("if else: %d\n", r1);
  if (r3 <= r4) {
    r3 = r3 + 1;
  }
  printf("if block: %d\n", r3);
  if (r5 < r6)
    r5 = r5 + 1;
  printf("if single stmt: %d\n", r5);
  if (r7 == r8)
    r7 = r7 + 1;
  else
    // 测试else与距离它最近的尚未配对的if语句结合的情况
    r8 = r8 + 1;
  printf("else match: %d %d\n", r7, r8);
  int i = 0;
  int total = 0;
  while (i < 10) {
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
  printf("while continue break: %d %d\n", i, total);
  int k = 0;
  while (k < 2)
    k = k + 1;
  printf("single stmt while: %d\n", k);
  switch (c) {
    // 测试switch语句作用于整型表达式的情况
    case 1:
      c = c + 1;
      break;
    case 2:
      c = c + 2;
    case 3:
      // 测试case分支末尾没有break时继续执行下一个case的情况
      break;
    case 4:
    case 5:
      // 测试多个case标签共用同一段语句的情况
      c = c + 5;
      break;
    default:
      // 测试default分支在所有case都不匹配时执行的情况
      c = c + 6;
  }
  printf("int switch: %d\n", c);
  switch (x) {
    // 测试switch语句作用于字符类型表达式且case标签为字符常量的情况
    case 'a':
      x = 'm';
      break;
    case 'b':
    case 'c':
      x = 'n';
      break;
    default:
      x = 'o';
  }
  printf("char switch: %c\n", x);
  switch (r1) {
    // 测试switch语句中不包含任何case分支的情况
  }
  printf("empty switch: %d\n", r1);
  side = 0;
  if (0 && side_effect()) {
    // 测试逻辑与运算中左操作数为假时右操作数不被求值的情况
    a = a + 100;
  }
  printf("and skip: %d\n", side);
  if (1 || side_effect()) {
    // 测试逻辑或运算中左操作数为真时右操作数不被求值的情况
    ;
  }
  printf("or skip: %d\n", side);
  if (a != b && b < c || a != b && c > 0) {
    // 测试逻辑与、逻辑或组合的情况
    a = a + 1;
  }
  printf("nested logic: %d\n", a);
  if (x < y) {
    // 测试字符类型之间进行关系运算的情况
    ;
  }
  if (!(a - b)) {
    // 测试逻辑非运算符作用于括号内算术表达式的情况
    a = a + 1;
  }
  printf("not paren: %d\n", a);
  if (in1 > 0 && side_effect()) {
    // 测试输入数据参与逻辑与运算且右操作数被求值的情况
    a = a + 1;
  }
  printf("input and: %d %d\n", side, a);
  int cnt1 = counter();
  int cnt2 = counter();
  printf("static counter: %d %d\n", cnt1, cnt2);
  printf("multi format: %d %c %s\n", r1, r15, cs);
  // 测试printf语句中同时使用整型、字符型与字符串格式字符的情况
  printf("global string: %s\n", gsa);
  printf("global const: %d %d %d\n", gcexp, gcn, gclen);
  // 测试读取全局常量的值的情况
  printf("global vars: %d %d %d\n", gva, gvb, gvc);
  // 测试读取全局变量的值的情况，未初始化的全局变量为零
  printf("const char: %c %c %c\n", gcx, gcy, gcz);
  printf("global array: %d %d %d\n", gia[0], gib[0], gib3[0]);
  printf("global char vars: %d %d %d\n", (int)gda, (int)gdb, (int)gdc);
  // 测试全局字符变量声明与读取的情况，未初始化的全局字符变量为零
  printf("const char list: %c\n", gccarr[1]);
  printf("const char string: %s\n", gcstr);
  printf("global char list: %c\n", gsb[2]);
  printf("const empty init: %d\n", gcarr2[0]);
  printf("const part init: %d\n", gcarr3[0]);
  printf("final: %d %d %d\n", a, b, i);
  return 0;
}
