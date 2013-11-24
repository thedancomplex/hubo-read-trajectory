#include <hubo-read-trajectory-as-func.h>


int main(){
  int mode=1;
  bool compliance=false;
  bool pause=false;

  char *s = "motion_043013_huboach_test.txt";//  "left_elbow.txt";

  printf("starting \n");
  runTrajFunction(s, mode, compliance, pause);

}
