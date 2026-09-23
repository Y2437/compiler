const int size = 5;
const char newline_ch = '\n';
// 测试字符常量中使用转义字符的情况
const int weights[5] = {1, 2, 3, 4, 5};
int buffer[5] = {9, 8, 7, 6, 5};
char name[8] = "sysy";
// 测试编译单元中只包含声明与主函数定义而不存在其他函数定义的情况

int main() {
  printf("24373169\n");
  // 测试printf语句只包含字符串常量的情况，此处输出学号
  int menu = get_int();
  // 测试switch表达式的值来源于get_int输入的情况
  char grade = get_char();
  // 测试字符变量的值来源于get_char输入的情况
  printf("input values: %d %c\n", menu, grade);
  int local[5];
  // 测试局部数组声明时不给出初始值的情况
  int i = 0;
  while (i < size) {
    // 测试在条件表达式中读取常量标识符的值的情况
    local[i] = buffer[i] + weights[i];
    // 测试两个全局数组的元素相加后存入局部数组的情况
    i = i + 1;
  }
  printf("array add: %d %d %d\n", local[0], local[2], local[4]);
  int tmp = local[0];
  local[0] = local[1];
  local[1] = tmp;
  // 测试通过临时变量交换两个数组元素的情况
  printf("swap inline: %d %d\n", local[0], local[1]);
  int acc = 0;
  i = 0;
  while (i < 5) {
    acc = acc + local[i] * weights[i];
    // 测试数组元素参与乘法和加法混合运算的情况
    i = i + 1;
  }
  printf("weighted sum: %d\n", acc);
  char letter = name[1];
  // 测试读取全局字符数组的元素的情况
  printf("array pick: %c\n", letter);
  switch (menu) {
    // 测试switch表达式为普通变量的情况
    case 1: {
      // 测试case分支中使用语句块的情况
      int inside = 10;
      printf("case block: %d\n", inside);
      break;
    }
    case 2:
      printf("case two\n");
      // 测试case分支中执行printf语句的情况
      break;
    case 3:
      printf("case three\n");
      break;
    default:
      printf("case default\n");
  }
  switch (grade) {
    // 测试switch语句作用于字符类型表达式的情况
    case 'a':
      grade = 'c';
      break;
    case 'b': {
      char nested = 'd';
      // 测试在case语句块内部声明局部字符变量的情况
      grade = nested;
      break;
    }
    default:
      grade = 'e';
  }
  printf("char switch: %c\n", grade);
  int count = 0;
  int j = 0;
  while (j < 20) {
    j = j + 1;
    if (j % 3 == 0) {
      continue;
    }
    if (j > 12) {
      break;
      // 测试break语句跳出while循环的情况
    }
    count = count + 1;
  }
  printf("while continue break: %d %d\n", j, count);
  int probe = 0;
  while (probe < 4)
    // 测试循环体为单条赋值语句的while循环的情况
    probe = probe + 2;
  printf("single stmt while: %d\n", probe);
  if (letter == 'y') {
    // 测试字符类型变量与字符常量进行相等性比较的情况
    count = count + 100;
  } else {
    count = count - 1;
  }
  printf("char equal: %d\n", count);
  if (grade != 'e')
    // 测试字符类型变量与字符常量进行不相等比较的情况
    count = count + 7;
  printf("char not equal: %d\n", count);
  int digits[3] = {1};
  printf("part init: %d %d %d\n", digits[0], digits[1], digits[2]);
  int empty_init[2] = {};
  printf("empty init: %d %d\n", empty_init[0], empty_init[1]);
  digits[1] = digits[0] * 10;
  digits[2] = (int)letter;
  // 测试将字符类型值显式转换后存入整型数组元素的情况
  printf("char to int array: %d %d\n", digits[1], digits[2]);
  printf("escape char: %d\n", (int)newline_ch);
  // 测试printf输出字符常量显式转换后的整型值的情况
  printf("menu after switch: %d\n", menu);
  // 测试输入数据经过switch语句之后的输出情况
  return 0;
}
