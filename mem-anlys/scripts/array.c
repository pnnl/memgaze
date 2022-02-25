#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#define TYPE int
//#define TYPE long

//size_t SIZE = 1000; /* 1000 */
//size_t SIZE = 2000; /* 2K */
size_t SIZE = 10000; /* 10K */
//size_t SIZE = 100000; /* 100K */
//size_t SIZE = 500000; /* 500K */
//size_t SIZE = 1000000; /* 1M */
//size_t SIZE = 100000000; /* 100 M */
//size_t SIZE =  2000000; /* 2 M */
//size_t SIZE =  10000000; /* 10 M */
//size_t SIZE = 10000000000; /* 10000 M */

TYPE __attribute__ ((noinline))
ubench_1D_Ind_x1_shfl(TYPE* a, TYPE* idx, size_t a_len);
////***************************************************************************
//
//void __attribute__ ((noinline))
//init_random(TYPE* array, size_t a_len) {
//  for (size_t i = 0; i < a_len; i++) {
//    array[i] = (random() % a_len);
//  }
//}
//
void __attribute__ ((noinline))
init_shuffle(TYPE* array, size_t a_len) {
  for (size_t i = 0; i < a_len; i++) {
    array[i] = i;
  }

  for (size_t i = 0; i < a_len - 1; i++) {
    size_t j = i + random() / (RAND_MAX / (a_len - i) + 1);
    int t = array[j];
    array[j] = array[i];
    array[i] = t;
  }
}

////***************************************************************************
////
////***************************************************************************
//
//TYPE __attribute__ ((noinline))
//ubench_1D_Str8_x1(TYPE* a,  size_t a_len) {
//  for (size_t i = 0; i < a_len; i=i+8) {
//    *a = a[i];
//  }
//  return a[a_len - 1];
//}
//
//
//TYPE __attribute__ ((noinline))
//ubench_1D_Str8_x2(TYPE* a,  size_t a_len) {
//   for (size_t j = 0; j < 2; j++) {
//      for (size_t i = 0; i < a_len; i=i+8 ) {
//         *a = a[i];
//      }
//   }
//   return a[a_len - 1];
//}
//
//
//TYPE __attribute__ ((noinline))
//ubench_1D_Str1_x1(TYPE* a,  size_t a_len) {
////  long long int address;
////  __builtin_ia32_ptwrite64(0xdeadbeef);
//  for (size_t i = 0; i < a_len; i++ ) {
////  address = (long long int)(&(a[i]));
////  __builtin_ia32_ptwrite64(address);
////  __builtin_ia32_ptwrite64(i);
//    *a = a[i];
//    if (i<2)
//      ubench_1D_Str8_x1(a, SIZE-i);
//  }
////  __builtin_ia32_ptwrite64(0);
//  return a[a_len - 1];
//}
//
//
TYPE __attribute__ ((noinline))
ubench_1D_Str1_x2(TYPE* a, TYPE* b, size_t a_len) {
  int temp = 0;
   for (size_t j = 0 ; j < 2 ; j++) {
      for (size_t i = 0; i < a_len; i++ ) {
         temp = a[i];
         usleep(1);
      }
      ubench_1D_Ind_x1_shfl(a, b, SIZE);
      //ubench_1D_Str8_x2(a, SIZE);
   }
   return temp;
}
//
//
////***************************************************************************
////
////***************************************************************************
//
//
TYPE __attribute__ ((noinline))
ubench_1D_Ind_x1_shfl(TYPE* a, TYPE* idx, size_t a_len) {
  int temp = 0;
  for (size_t i = 0; i < a_len; i++) {
    temp  = a[idx[i]];
    usleep(1);
  }
  return temp;
}
//
//
//TYPE __attribute__ ((noinline))
//ubench_1D_Ind_x1_rand(TYPE* a, TYPE* idx, size_t a_len) {
//  for (size_t i = 0; i < a_len; i++) {
//    a[i]  = a[idx[i]];
//    if (i <2)
//      ubench_1D_Str8_x1(a, SIZE);
//  }
//  return a[a_len - 1];
//}
//
//
//TYPE __attribute__ ((noinline))
//ubench_1D_Ind_x2(TYPE* a, TYPE* idx, size_t a_len) {
//   for (size_t j = 0 ; j < 2 ; j++) { 
//      for (size_t i = 0; i < a_len; i++) {
//         a[i]  = a[idx[i]];
//      }
//      ubench_1D_Str1_x2(a, SIZE);
//   }
//   return a[a_len - 1];
//}
//
//
//TYPE __attribute__ ((noinline))
//ubench_1D_Ind_halfx1(TYPE* a, TYPE* idx, size_t a_len) {
//  for (size_t i = 0; i < a_len; i++) {
//    a[i]  = a[idx[i]];
//  }
//  return a[a_len - 1];
//}
//
//
//TYPE __attribute__ ((noinline))
//ubench_1D_If_halfx1(TYPE* a, TYPE* b, TYPE* idx, size_t a_len) {
//  int t = -1;
//  for (size_t i = 0; i < a_len; i++) {
//    t *= -1;
//    if (t < 0){
//      a[i] = a[idx[i]];
//      printf("address of a[i]: %p, a[idx[i]]: %p, idx[i]: %p\n",&a[i],&a[idx[i]],&idx[i]);
//    }  else {
//      b[i] = b[i];
//    }
//  }
//
//  return a[a_len - 1];
//}


//***************************************************************************
//
//***************************************************************************

int
main(int argc, char* argv[] /*const char* envp[]*/) {
  long ubench_mask = 0xfff;
  int temp = 0;
  // -------------------------------------------------------
  // command line (argv[0] is program name)
  // -------------------------------------------------------
  // usage <me> [<hex-mask>]
  if (argc > 1) {
    ubench_mask = strtol(argv[1], NULL, 16);
  }
  //__builtin_ia32_ptwrite64(0x2);
  if (ubench_mask & 0x001) {
   // __builtin_ia32_ptwrite64(0x4);
    temp++;
  }
  if(ubench_mask & 0x002){
   // __builtin_ia32_ptwrite64(0x8);
    temp+=2;
  }else {
   // __builtin_ia32_ptwrite64(0x10);
    temp+=3;
  }
  TYPE* a = (TYPE*)malloc(sizeof(TYPE)*SIZE);
//  TYPE* b = (TYPE*)malloc(sizeof(TYPE)*SIZE);

//  TYPE* idx_r = (TYPE*)malloc(sizeof(TYPE)*SIZE);
  TYPE* idx_s = (TYPE*)malloc(sizeof(TYPE)*SIZE);

  // idx_r: random indices between [1..SIZE]
//  init_random(idx_r, SIZE);

  // idx_s: shuffled (permuted) indices
  init_shuffle(idx_s, SIZE);
 
  //----------------------------------------------------------
  // 1D/strided access
  //----------------------------------------------------------

  //__builtin_ia32_ptwrite64(32);
//  if (ubench_mask & 0x001) {
//  //  __builtin_ia32_ptwrite64(64);
//    ubench_1D_Str1_x1(a, SIZE);
//  }else{
//  // __builtin_ia32_ptwrite64(128);
//  }
//  //__builtin_ia32_ptwrite64(256);
//  if (ubench_mask & 0x002) {
    ubench_1D_Str1_x2(a, idx_s, SIZE);
//  }
//  if (ubench_mask & 0x004) {
//    ubench_1D_Str8_x1(a, SIZE);
//  }
//  if (ubench_mask & 0x008) {
//    ubench_1D_Str8_x2(a, SIZE);
//  }
//
//  //----------------------------------------------------------
//  // 1D/indirect access
//  //----------------------------------------------------------
//
//  if (ubench_mask & 0x010) {
//    ubench_1D_Ind_x1_shfl(a, idx_s, SIZE);
//  }
//  if (ubench_mask & 0x020) {
//    ubench_1D_Ind_x1_rand(a, idx_r, SIZE); // not unused in results
//  }
//  if (ubench_mask & 0x040) {
//    ubench_1D_Ind_x2     (a, idx_s, SIZE);
//  }
//  if (ubench_mask & 0x080) {
//    ubench_1D_Ind_halfx1 (a, idx_s, SIZE/2);
//  }
//  if (ubench_mask & 0x100) {
//    ubench_1D_If_halfx1  (a, b, idx_s, SIZE);
//  }

  //----------------------------------------------------------
  //---------------------------------------------------------- 
  free(a);
//  free(b);
//  free(idx_r);
  free(idx_s);
  
  return 0;
}
