// 测试编译单元中不存在声明而只包含函数定义与主函数定义的情况

void reset() {
  // 测试返回类型为void的函数中不带表达式的return语句的情况
  return;
}

int identity(int v) {
  // 测试函数体中只有一条return语句的情况
  return v;
}

int max3(int x, int y, int z) {
  // 测试函数形参表中花括号内重复多次的情况
  int m = x;
  if (y > m) {
    m = y;
  }
  if (z > m) {
    m = z;
  }
  return m;
}

int main() {
  printf("24373169\n");
  // 测试printf语句只包含字符串常量的情况，此处输出学号
  int width = get_int();
  // 测试get_int函数读取输入并赋值给变量的情况
  int height = get_int();
  printf("input values: %d %d\n", width, height);
  int marker;
  // 测试局部变量声明时不给出初始值的情况
  int n = 1;
  int sum = 0;
  int product = 1;
  while (n <= 10) {
    // 测试条件为单个关系表达式的while循环的情况
    sum = sum + n;
    product = product * 2;
    if (product >= 32) {
      product = product / 2;
    }
    n = n + 3;
  }
  printf("while loop: %d %d %d\n", sum, product, n);
  int neg = -sum;
  // 测试对变量使用单目取负运算符的情况
  printf("unary minus: %d\n", neg);
  int pos = +n;
  // 测试对变量使用单目取正运算符的情况
  printf("unary plus: %d\n", pos);
  int mixed = sum - neg % 7 + product / 3;
  // 测试加减运算与乘除模运算混合出现时的优先级的情况
  printf("mixed priority: %d\n", mixed);
  int grouped = (sum + product) * (n - 1);
  // 测试括号改变运算优先级的情况
  printf("paren priority: %d\n", grouped);
  if (sum == 0)
    // 测试if语句中相等性条件成立的情况
    marker = 1;
  else
    // 测试else与距离它最近的尚未配对的if语句结合的情况
    marker = 2;
  printf("else match: %d\n", marker);
  if (sum != 0) {
    if (product < 100) {
      marker = 3;
    } else {
      marker = 4;
    }
  }
  printf("nested if: %d\n", marker);
  int r = max3(sum, product, n);
  // 测试函数调用时传递多个实参的情况
  printf("multi params: %d\n", r);
  identity(r);
  // 测试函数调用表达式作为语句的情况
  printf("identity call: %d\n", identity(r));
  reset();
  {
    // 测试语句块作为语句的情况
    int inner = sum + r;
    printf("block item: %d\n", inner);
    {
      // 测试多层嵌套语句块的情况
      int deeper = inner * 2;
      printf("nested block: %d\n", deeper);
    }
  }
  int flag = 0;
  while (flag < 5) {
    flag = flag + 1;
    if (flag == 2) {
      continue;
      // 测试continue语句位于嵌套语句块中的情况
    }
    if (flag > 4) {
      break;
    }
  }
  printf("continue break: %d\n", flag);
  if (!flag) {
    // 测试逻辑非运算符作用于变量的情况
    marker = 5;
  }
  printf("not on var: %d\n", marker);
  if (!(sum - 20)) {
    // 测试逻辑非运算符作用于括号内算术表达式的情况
    marker = 6;
  }
  printf("not on paren: %d\n", marker);
  int area = width * height + sum;
  // 测试两个输入数据参与乘法运算后与局部变量相加的情况
  printf("input compute: %d\n", area);
  printf("mixed grouped: %d\n", mixed + grouped);
  return 0;
}
