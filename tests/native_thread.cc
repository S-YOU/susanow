
#include <stdio.h>
#include <susanow.h>

size_t num1 = 1;
size_t num2 = 2;

void test(void* arg)
{
  size_t* n = (size_t*)arg;
  for (size_t i=0; i<3; i++) {
    printf("test arg=%zd\n", *n);
    ssn_sleep(1000);
  }
}

int main(int argc, char** argv)
{
  ssn_init(argc, argv);

  ssn_native_thread_launch(test, &num1, 1);
  ssn_sleep(1000);
  ssn_native_thread_launch(test, &num2, 2);

  ssn_fin();
}