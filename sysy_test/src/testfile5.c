int side = 0;
// 测试用于观察短路求值副作用的全局变量的情况

int side_effect() {
  // 测试带有副作用的函数参与条件表达式求值的情况
  side = side + 1;
  return 1;
}

int zero_func() {
  side = side + 10;
  return 0;
}

int main() {
  printf("24373169\n");
  // 测试printf语句只包含字符串常量的情况，此处输出学号
  int in1 = get_int();
  // 测试get_int函数读取的输入参与条件判断的情况
  int in2 = get_int();
  printf("input values: %d %d\n", in1, in2);
  int a = 1, b = 2, c = 3;
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
  if (a < b && side_effect()) {
    // 测试逻辑与运算中左操作数为真时右操作数被求值的情况
    a = a + 1;
  }
  printf("and eval: %d %d\n", side, a);
  if (a > b || zero_func()) {
    // 测试逻辑或运算中左操作数为假时右操作数被求值的情况
    b = b + 1;
  }
  printf("or eval: %d %d\n", side, b);
  if (a != b && b < c || a != b && c > 0) {
    // 测试逻辑与、逻辑或组合的条件表达式的情况
    a = a + 1;
  }
  printf("nested logic: %d\n", a);
  int r1 = 0;
  if (a < b && b < c) {
    // 测试逻辑与运算组成条件表达式的情况
    r1 = 1;
  }
  printf("land value: %d\n", r1);
  int r2 = 0;
  if (a > b || b < c) {
    // 测试逻辑或运算组成条件表达式的情况
    r2 = 1;
  }
  printf("lor value: %d\n", r2);
  int count = 0;
  int i = 0;
  while (i < 10 && count < 3) {
    // 测试while循环条件中使用逻辑与运算的情况
    i = i + 1;
    if (i % 2 == 0) {
      continue;
    }
    count = count + 1;
  }
  printf("while and: %d %d\n", i, count);
  int j = 0;
  while (j > 5 || j < 3) {
    // 测试while循环条件中使用逻辑或运算的情况
    j = j + 1;
  }
  printf("while or: %d\n", j);
  if (a == 2 && b == 3 && c == 3) {
    // 测试逻辑与运算连续结合多个操作数的情况
    c = c + 1;
  }
  printf("and chain: %d\n", c);
  if (a == 0 || b == 0 || c == 4) {
    // 测试逻辑或运算连续结合多个操作数的情况
    c = c + 1;
  }
  printf("or chain: %d\n", c);
  side = 0;
  int complex_cond = 0;
  if (a < b && side_effect() || b > c && side_effect()) {
    // 测试逻辑与运算与逻辑或运算混合时的优先级的情况，逻辑与的优先级高于逻辑或
    complex_cond = 1;
  }
  printf("logic priority: %d %d\n", complex_cond, side);
  if (a == b) {
    // 测试相等性判断作为条件表达式的情况
    a = a + 1;
  }
  printf("equal cond: %d\n", a);
  side = 0;
  if (in1 < in2 && side_effect()) {
    // 测试输入数据参与逻辑与运算且右操作数被求值的情况
    a = a + 1;
  }
  printf("input and: %d %d\n", side, a);
  if (in1 > in2 || side_effect()) {
    // 测试输入数据参与逻辑或运算且右操作数被求值的情况
    b = b + 1;
  }
  printf("input or: %d %d\n", side, b);
  printf("final abc: %d %d %d\n", a, b, c);
  return 0;
}
