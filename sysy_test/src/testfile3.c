const int gclen = 4;
// 测试只声明一个常量的常量声明的情况
const int gcarr[3] = {4, 5, 6};
// 测试常量数组带有完整初值列表的情况
const int gcarr2[3] = {};
// 测试常量数组初值列表为空的情况
const int gcarr3[3] = {7};
// 测试常量数组初值列表只给出部分元素且剩余元素被置为零的情况
const char gcstr[6] = "hello";
// 测试字符常量数组使用字符串常量初始化的情况
const char gccarr[3] = {'p', 'q', 'r'};
// 测试字符常量数组使用字符常量列表初始化的情况
int gia[3] = {1, 2, 3};
// 测试全局整型数组初始化的情况
int gib[3];
// 测试全局数组不给出初始值而被置为零的情况
char gsa[6] = "hello";
// 测试全局字符数组使用字符串常量初始化的情况
char gsb[3] = {'a', 'b', 'c'};
char gsc[3];

int sum_array(int arr[], int n) {
  // 测试整型数组作为函数形参的情况
  int i = 0;
  int s = 0;
  while (i < n) {
    s = s + arr[i];
    i = i + 1;
  }
  return s;
}

char shift_char(char c, char d) {
  // 测试返回类型为char的函数以及字符类型形参的情况
  return c + d;
  // 测试字符类型之间进行加法运算且结果仍为字符类型的情况
}

char first_char(char s[]) {
  // 测试字符数组作为函数形参的情况
  return s[0];
}

void fill(int arr[], int value) {
  // 测试通过数组形参间接修改实参数组元素的情况
  arr[0] = value;
  return;
}

int main() {
  printf("24373169\n");
  // 测试printf语句只包含字符串常量的情况，此处输出学号
  int n = get_int();
  // 测试get_int函数读取整型输入的情况
  char ch = get_char();
  // 测试get_char函数读取单个字符输入的情况
  char inbuf[16];
  get_string(inbuf, 16);
  // 测试get_string函数读取一行字符串输入到字符数组的情况
  printf("get int: %d\n", n);
  printf("get char: %c\n", ch);
  printf("get string: %s", inbuf);
  int arr[4] = {4, 5, 6, 7};
  int arr2[3] = {8};
  // 测试局部整型数组初值列表只给出部分元素的情况
  printf("part init: %d %d %d\n", arr2[0], arr2[1], arr2[2]);
  int arr3[2] = {};
  // 测试局部整型数组初值列表为空的情况
  int arr4[2];
  // 测试局部数组不给出初始值的情况
  char cs[8] = "sysy\n";
  // 测试局部字符数组使用包含换行转义的字符串常量初始化的情况
  char cs2[4] = "ab";
  // 测试数组长度大于字符串长度时剩余元素被置为零的情况
  printf("string pad: %d %d\n", (int)cs2[2], (int)cs2[3]);
  char cs3[3] = {'m', 'n', 'o'};
  printf("char list init: %c %c %c\n", cs3[0], cs3[1], cs3[2]);
  const int care[3] = {10, 20, 30};
  const char cstr[5] = "test";
  int convarr[100];
  // 测试使用整型常量作为数组长度的情况
  convarr[0] = (int)'d';
  printf("cast length array: %d\n", convarr[0]);
  printf("const array init: %d %d\n", care[0], care[2]);
  printf("empty list init: %d %d\n", arr3[0], arr3[1]);
  int idx = 1;
  arr[0] = arr[0] + arr[idx];
  // 测试使用变量作为数组下标的情况
  printf("var index: %d\n", arr[0]);
  arr[1] = arr[1] + n;
  // 测试输入数据参与数组元素运算的情况
  printf("input to array: %d\n", arr[1]);
  arr4[0] = 1;
  arr4[1] = arr4[0] + 2;
  printf("array assign: %d %d\n", arr4[0], arr4[1]);
  cs[0] = 'k';
  // 测试对字符数组元素进行赋值的情况
  cs[idx] = 'q';
  cs[2] = ch;
  // 测试输入字符写入字符数组元素的情况
  printf("char array set: %s\n", cs);
  char x = 'a', y = 'b', z;
  // 测试字符类型变量声明的情况
  z = shift_char(x, '\0');
  // 测试函数实参中使用字符常量的情况
  printf("char add: %c\n", z);
  z = first_char(cs);
  // 测试传递字符数组给函数的情况
  printf("array param char: %c\n", z);
  int total = sum_array(arr, 4);
  // 测试传递整型数组给函数的情况
  printf("array param int: %d\n", total);
  fill(arr4, 9);
  printf("modify via param: %d\n", arr4[0]);
  int elem = arr[2];
  // 测试读取数组元素作为表达式操作数的情况
  printf("array elem: %d\n", elem);
  int c = 3;
  switch (c) {
    // 测试switch语句中多个case分支的情况
    case 1:
      c = c + 1;
      break;
      // 测试break语句跳出switch结构的情况
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
    // 测试switch表达式为字符类型且case标签为字符常量的情况
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
  switch (elem) {
    // 测试switch语句中不包含任何case分支的情况
  }
  printf("empty switch: %d\n", elem);
  int i = 0;
  while (i < 2) {
    // 测试在循环体内访问数组元素的情况
    arr4[i] = arr4[i] + i;
    i = i + 1;
  }
  printf("loop array: %d %d\n", arr4[0], arr4[1]);
  if (x < y) {
    // 测试字符类型之间进行关系运算的情况
    z = x;
  }
  printf("char compare: %c\n", z);
  int ci = (int)x;
  // 测试将字符类型显式转换为整型的情况
  printf("char to int: %d\n", ci);
  char back = (char)(ci + 1);
  // 测试将整型显式转换为字符类型的情况
  printf("int to char: %c\n", back);
  printf("global string: %s\n", gsa);
  printf("const string: %s\n", cstr);
  printf("const array: %d %d %d\n", gcarr[0], gcarr[1], gcarr[2]);
  printf("const part init: %d %d %d\n", gcarr3[0], gcarr3[1], gcarr3[2]);
  printf("const empty init: %d\n", gcarr2[0]);
  printf("global zero: %d\n", gib[0]);
  printf("single const: %d\n", gclen);
  // 测试在表达式中读取常量标识符的值的情况
  printf("const char list: %c %c\n", gccarr[0], gccarr[2]);
  // 测试读取字符常量数组元素的情况
  printf("const char string: %s\n", gcstr);
  printf("global int array: %d\n", gia[2]);
  // 测试读取全局整型数组元素的情况
  printf("global char list: %c\n", gsb[1]);
  printf("global char zero: %d\n", (int)gsc[0]);
  // 测试全局字符数组不给出初始值而被置为零的情况
  return 0;
}
