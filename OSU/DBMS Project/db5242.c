/*
  CSE 5242 Project 2, Fall 2023

  See class project handout for more extensive documentation.

  https://stackoverflow.com/questions/19068705/undefined-reference-when-calling-inline-function
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <asm/unistd.h>
#include <immintrin.h>

/* uncomment out the following DEBUG line for debug info, for experiment comment the DEBUG line  */
//#define DEBUG


/* compare two int64_t values - for use with qsort */
static int compare(const void *p1, const void *p2)
{
  int a,b;
  a = *(int64_t *)p1;
  b = *(int64_t *)p2;
  if (a<b) return -1;
  if (a==b) return 0;
  return 1;
}

/* initialize searches and data - data is sorted and searches is a random permutation of data */
int init(int64_t* data, int64_t* searches, int count)
{
  for(int64_t i=0; i<count; i++){
    searches[i] = random();
    data[i] = searches[i]+1;
  }
  qsort(data,count,sizeof(int64_t),compare);
}

/* initialize outer probes of band join */
int band_init(int64_t* outer, int64_t size)
{
  for(int64_t i=0; i<size; i++){
    outer[i] = random();
  }
}

inline int64_t simple_binary_search(int64_t* data, int64_t size, int64_t target)
{
  int64_t left=0;
  int64_t right=size;
  int64_t mid;

  while(left<=right) {
    mid = (left + right)/2;   /* ignore possibility of overflow of left+right */
    if (data[mid]==target) return mid;
    if (data[mid]<target) left=mid+1;
    else right = mid-1;
  }
  return -1; /* no match */
}

inline int64_t low_bin_search(int64_t* data, int64_t size, int64_t target)
{
  /* this binary search variant
     (a) does only one comparison in the inner loop
     (b) doesn't require an exact match; instead it returns the index of the first key >= the search key.
         That's good in a DB context where we might be doing a range search, and using binary search to
	 identify the first key in the range.
     (c) If the search key is bigger than all keys, it returns size.
  */
  int64_t left=0;
  int64_t right=size;
  int64_t mid;

  printf("Low_bin_search\n");
  while(left<right) {
    mid = (left + right)/2;   /* ignore possibility of overflow of left+right */
    if (data[mid]>=target)
      right=mid;
    else
      left=mid+1;
  }
  return right;
}

//#define ARDEBUG
inline int64_t low_bin_nb_arithmetic(int64_t* data, int64_t size, int64_t target)
{
  /* this binary search variant
     (a) does no comparisons in the inner loop by using multiplication and addition to convert control dependencies
         to data dependencies
     (b) doesn't require an exact match; instead it returns the index of the first key >= the search key.
         That's good in a DB context where we might be doing a range search, and using binary search to
	 identify the first key in the range.
     (c) If the search key is bigger than all keys, it returns size.
  */
  int64_t left=0;
  int64_t right=size;
  int64_t mid;

  #ifdef ARDEBUG
    printf("low_bin_nb_arith\n");
  #endif 
  //0^1 = 1 and 1^1 = 0
  while(left<right) {
    mid = (left + right) / 2; //get middle
    int64_t flag = data[mid] >= target; //data[mid] >= target ? 1 : 0 (if target value is left side of the middle index flag = 1 else 0)
    left = flag * left + (flag^1) * (mid+1);// if flag == 1 assign previous left value (left index stays same) else assign mid+1 value to divide the array into half
    right = flag * mid + (flag^1) * right;// if flag == 1 assign mid value to divide array into half else assign previous right value (right index stays same)
    #ifdef ARDEBUG
        printf("mid: %d left: %d right: %d\n", mid, left, right);
    #endif 
  }
  return right;
}

//#define MASTDEBUG
inline int64_t low_bin_nb_mask(int64_t* data, int64_t size, int64_t target)
{
  /* this binary search variant
     (a) does no comparisons in the inner loop by using bit masking operations to convert control dependencies
         to data dependencies
     (b) doesn't require an exact match; instead it returns the index of the first key >= the search key.
         That's good in a DB context where we might be doing a range search, and using binary search to
	 identify the first key in the range.
     (c) If the search key is bigger than all keys, it returns size.
  */
  int64_t left=0;
  int64_t right=size;
  int64_t mid;

  #ifdef MASTDEBUG
    printf("low_bin_nb_mask\n");
  #endif
  // ~(-1) = 0  and ~(0) = -1
  //-1 & 3 = 3  and 0 & 3 = 0
  //0 | 3 = 3
  while(left<right) {
     //if-else using bitwise source: https://stackoverflow.com/questions/3798601/conditional-using-bitwise-operator
    mid = (left + right) / 2;   //get middle
    int64_t flag = (data[mid] >= target)-1;  //data[mid] >= target ? 0 : -1 (if target value is left side of middle index flag = 0 else =1)
    left = (flag & (mid+1)) | (~flag & left); // If flag == -1 assign mid+1 to divide the array into half else assign previous left value (left index stays same)
    right = (flag & right) | (~flag & mid); // if flag == -1 assign previous right value (right index stay same) else assing mid to divide the array into half
    #ifdef MASTDEBUG
        printf("mid: %d left: %d right: %d\n", mid, left, right);
    #endif
    //int64_t flag = (data[mid] >= target);  //data[mid] >= target ? 1 : 0
    // left = ((flag-1) & (mid+1)) | ((flag^1)-1 & left); 
    // right = ((flag-1) & right) | ((flag^1)-1 & mid); 
  }
  return right;
}

//#define XDEBUG
inline void low_bin_nb_4x(int64_t* data, int64_t size, int64_t* targets, int64_t* right)
{
  /* this binary search variant
     (a) does no comparisons in the inner loop by using bit masking operations instead
     (b) doesn't require an exact match; instead it returns the index of the first key >= the search key.
         That's good in a DB context where we might be doing a range search, and using binary search to
	 identify the first key in the range.
     (c) If the search key is bigger than all keys, it returns size.
     (d) does 4 searches at the same time in an interleaved fashion, so that an out-of-order processor
         can make progress even when some instructions are still waiting for their operands to be ready.

     Note that we're using the result array as the "right" elements in the search so no need for a return statement
  */
  int64_t left[4]={0,0,0,0};
  int64_t mid[4];
  right[0]=right[1]=right[2]=right[3]=size;
  
  #ifdef XDEBUG
    printf("low_bin_nb_4x\n");
  #endif
  int64_t bit_size = 4;
  int cnt = 0;
  //outer loop to check termination condition like above (e.g left < right)
  while (left[0]<right[0] || left[1]<right[1] || left[2]<right[2] || left[3]<right[3]) {
    int i;
    #ifdef XDEBUG
        printf("cnt: %d\n", cnt);
    #endif
    //inner loop to perform 4 search concurrently 
    for (i = 0; i < bit_size; i++) {
      //if-else using bitwise source: https://stackoverflow.com/questions/3798601/conditional-using-bitwise-operator
      mid[i] = (left[i] + right[i]) / 2;   //get middle
      int64_t flag = (data[mid[i]] >= targets[i])-1;  //data[mid] >= target ? 0 : -1  (if target value is left side of middle index flag = 0 else =1)
      left[i] = (flag & (mid[i]+1)) | (~flag & left[i]);  // If flag == -1 assign mid+1 to divide the array into half else assign previous left value (left index stays same)
      right[i] = (flag & right[i]) | (~flag & mid[i]); // if flag == -1 assign previous right value (right index stay same) else assing mid to divide the array into half
      #ifdef XDEBUG
            printf("mid: %d left: %d right: %d\n", mid[i], left[i], right[i]);
      #endif
    
      //int64_t flag = (data[mid[i]] > targets[i]);  //data[mid] >= target ? 1 : 0
      // left[i] = ((flag-1) & (mid[i]+1)) | ((flag^1)-1 & left[i]);
      // right[i] = ((flag-1) & right[i]) | ((flag^1)-1 & mid[i]); 
      //break;
    }
    cnt+=1;
  }
}


/* The following union type is handy to output the contents of AVX512 data types */
union int8x4 {
  __m256i a;
  int64_t b[4];
};

void printavx(char* name, __m256i v) {
  union int8x4 n;

  n.a=v;
  printf("Value in %s is [%ld %ld %ld %ld ]\n",name,n.b[0],n.b[1],n.b[2],n.b[3]);
}

/*
 * Optinal for using AVX-512

  union int8x8 {
    __m512i a;
    int64_t b[8];
  };

  void printavx512(char* name, __m512i v) {
    union int8x4 n;

    n.a=v;
    printf("Value in %s is [%ld %ld %ld %ld %ld %ld %ld %ld ]\n",name,n.b[0],n.b[1],n.b[2],n.b[3]);
  }

 */

//#define SIMDDEBUG
inline void low_bin_nb_simd(int64_t* data, int64_t size, __m256i target, __m256i* result)
{
  /* this binary search variant
     (a) does no comparisons in the inner loop by using bit masking operations instead
     (b) doesn't require an exact match; instead it returns the index of the first key >= the search key.
         That's good in a DB context where we might be doing a range search, and using binary search to
	 identify the first key in the range.
     (c) If the search key is bigger than all keys, it returns size.
     (d) does 4 searches at the same time using AVX2 intrinsics

     See https://www.intel.com/content/www/us/en/docs/cpp-compiler/developer-guide-reference/2021-8/intrinsics-for-avx2.html
     for documentation of the AVX512 intrinsics

     Note that we're using the result array as the "right" elements in the search, and that searchkey is being passed
     as an __m256i value rather than via a pointer.
  */

 __m256i aleft = _mm256_set1_epi64x(0);
  __m256i aright = _mm256_set1_epi64x(size);
  __m256i amid;

  __m256i ones = _mm256_set1_epi64x(1);
  __m256i amask;
  __m256i datavec;

#ifdef SIMDDEBUG
  printf("low_bin_nb_simd\n");
#endif
  //AVX Intrinsic Guide: https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html 
  __m256i cmp = _mm256_cmpgt_epi64(aright, aleft);
  //Check if all elements of vector is 0 using testz: https://stackoverflow.com/questions/32072169/could-i-compare-to-zero-register-in-avx-correctly + https://stackoverflow.com/a/32120039 
  //Check all elements in cmp are 0 (if value of cmp is 0 it means left > right else -1)
  while(_mm256_testz_si256(cmp, cmp) == 0){
    amid = _mm256_srli_epi64(_mm256_add_epi64(aleft, aright), 1); //get middle
    __m256i amid_one = _mm256_add_epi64(amid, ones); //get middle+1 vector
    //Extract matching values of data using AVX index vector using i64gather: https://stackoverflow.com/questions/51128005/what-do-you-do-without-fast-gather-and-scatter-in-avx2-instructions 
    datavec = _mm256_i64gather_epi64((long long*)data, amid, sizeof(int64_t));

    // inlining failed error for >= opeartion
    //__mmask8 flag = _mm256_cmpge_epi64_mask(datavec, target);    
    //Combine > operation and = operation to have >= operation: https://www.splashlearn.com/math-vocabulary/greater-than-or-equal-to
    __m256i cmpgt = _mm256_cmpgt_epi64(datavec, target); 
    __m256i cmpeq = _mm256_cmpeq_epi64(datavec, target); 
    __m256i flag = _mm256_or_si256(cmpgt, cmpeq); 

    //if-else using bitwise source: https://stackoverflow.com/questions/3798601/conditional-using-bitwise-operator
    aleft = _mm256_or_si256(_mm256_and_si256(flag, aleft),_mm256_andnot_si256(flag, amid_one)); // if falg == -1 assign left value (left index stays same) else assing mid+1 to divide the array into half.  
    aright = _mm256_or_si256(_mm256_and_si256(flag, amid),_mm256_andnot_si256(flag, aright)); // if flag == -1 assign mid value to divide array into half else assign previous right value (rihgt index stays same)
  
#ifdef SIMDDEBUG
    printavx("amid", amid);
    printavx("flag", flag);
    printavx("aright", aright);
    printavx("aleft", aleft);
    printf("\n\n");
#endif
    //break;
    cmp = _mm256_cmpgt_epi64(aright, aleft);
  }
  //aright = _mm256_sub_epi64(aright, ones);
  *result = aright;
}

void bulk_bin_search(int64_t* data, int64_t size, int64_t* searchkeys, int64_t numsearches, int64_t* results, int repeats)
{
  for(int j=0; j<repeats; j++) {
    /* Function to test a large number of binary searches

       we might need repeats>1 to make sure the events we're measuring are not dominated by various
       overheads, particularly for small values of size and/or numsearches

       we assume that we want exactly "size" searches, where "size" is the length if the searchkeys array
     */
    for(int64_t i=0;i<numsearches; i++) {
#ifdef DEBUG
      printf("Searching for %ld...\n",searchkeys[i]);
#endif

      // Uncomment one of the following to measure it
      //results[i] = low_bin_search(data,size,searchkeys[i]);
      results[i] = low_bin_nb_arithmetic(data,size,searchkeys[i]);
      //results[i] = low_bin_nb_mask(data,size,searchkeys[i]);

#ifdef DEBUG
      printf("Result is %ld\n",results[i]);
#endif
    }
  }
}

void bulk_bin_search_4x(int64_t* data, int64_t size, int64_t* searchkeys, int64_t numsearches, int64_t* results, int repeats)
{
  register __m256i searchkey_4x;

  for(int j=0; j<repeats; j++) {
    /* Function to test a large number of binary searches using one of the 8x routines

       we might need repeats>1 to make sure the events we're measuring are not dominated by various
       overheads, particularly for small values of size and/or numsearches

       we assume that we want exactly "size" searches, where "size" is the length if the searchkeys array
     */
    int64_t extras = numsearches % 4;
    for(int64_t i=0;i<numsearches-extras; i+=4) {
#ifdef DEBUG
      printf("Searching for %ld %ld %ld %ld  ...\n",
	     searchkeys[i],searchkeys[i+1],searchkeys[i+2],searchkeys[i+3]);
#endif

      // Uncomment one of the following depending on which routine you want to profile

      // Algorithm A
      //low_bin_nb_4x(data,size,&searchkeys[i],&results[i]);

      // Algorithm B
      searchkey_4x = _mm256_loadu_si256((__m256i *)&searchkeys[i]);
      low_bin_nb_simd(data,size,searchkey_4x,(__m256i *)&results[i]);

#ifdef DEBUG
      printf("Result is %ld %ld %ld %ld  ...\n",
	    results[i],results[i+1],results[i+2],results[i+3]);
#endif
    }
    /* a little bit more work if numsearches is not a multiple of 8 */
    for(int64_t i=numsearches-extras;i<numsearches; i++) {

      results[i] = low_bin_nb_mask(data,size,searchkeys[i]);

    }

  }
}


int64_t band_join(int64_t* inner, int64_t inner_size, int64_t* outer, int64_t outer_size, int64_t* inner_results, int64_t* outer_results, int64_t result_size, int64_t bound)
{
  /* In a band join we want matches within a range of values.  If p is the probe value from the outer table, then all
     reccords in the inner table with a key in the range [p-bound,p+bound] inclusive should be part of the result.

     Results are returned via two arrays. outer_results stores the index of the outer table row that matches, and
     inner_results stores the index of the inner table row that matches.  result_size tells you the size of the
     output array that has been allocated. You should make sure that you don't exceed this size.  If there are
     more results than can fit in the result arrays, then return early with just a prefix of the results in the result
     arrays. The return value of the function should be the number of output results.

  */
 //Declaring arrays to store left and right index of the range.
  int64_t *leftIndexArray = malloc(outer_size * sizeof(int64_t));
  int64_t *rightIndexArray = malloc(outer_size * sizeof(int64_t));
  if (!leftIndexArray || !rightIndexArray) {
    free(leftIndexArray);
    free(rightIndexArray); 
    // Memory allocation failed, so we return -1
    return -1; 
  }
  //Calcuating below values to find the multiplier and remainder.
  int multiplier = outer_size/4;
  int remainder = outer_size%4;

  int temp_1=0;

  while (temp_1<multiplier){
    // printf("Inside  muliplier with i->%d\n",temp_1);
    //using the below arrays to store the lower bound values temporarily after using low_bin_nb_mask function.
    int64_t lower_limit[4];
    int64_t upper_limit[4];

    for (int i =0;i<4;i++){
      lower_limit[i]=outer[4*temp_1+i]-bound;
      upper_limit[i]=outer[4*temp_1+i]+bound;
    }
    int64_t left_temp_Array[4];
    int64_t right_temp_Array[4];
    
    low_bin_nb_4x(inner,inner_size,lower_limit,left_temp_Array);
    low_bin_nb_4x(inner,inner_size,upper_limit,right_temp_Array);

    for (int i =0;i<4;i++){
      leftIndexArray[4*temp_1 + i]=left_temp_Array[i];
      rightIndexArray[4*temp_1 + i]=right_temp_Array[i];
    }
    // printf("Ending  muliplier with i->%d\n",temp_1);
    temp_1+=1;

  }

  for(int i = 0; i<remainder;i++){
    // printf("Entering remainder with i->%d\n",i);
    // printf("lower-> %d\n",outer[i]-bound);
    // printf("upper-> %d\n",outer[i]+bound);
    leftIndexArray[4*temp_1 + i]=low_bin_nb_mask(inner, inner_size, outer[4*temp_1 + i]-bound);//array containing left index in the range.
    rightIndexArray[4*temp_1 + i]=low_bin_nb_mask(inner,inner_size,outer[4*temp_1 + i]+bound);//array containing right index of the range.
    // printf("leftindex->%d \t\n rightindex->%d\t\n",leftIndexArray[i],rightIndexArray[i]);
    // printf("Exiting reminder loop with i->%d\n",i);
  }printf("\n");



/* ONLY WITH low_bin_nb_mask
  for(int i = 0; i<outer_size;i++){
    // printf("lower-> %d\n",outer[i]-bound);
    // printf("upper-> %d\n",outer[i]+bound);
    leftIndexArray[i]=low_bin_nb_mask(inner, inner_size, outer[i]-bound);//array containing left index in the range.
    rightIndexArray[i]=low_bin_nb_mask(inner,inner_size,outer[i]+bound);//array containing right index of the range.
    // printf("leftindex->%d \t\n rightindex->%d\t\n",leftIndexArray[i],rightIndexArray[i]);
  }printf("\n");
*/
// for(int i = 0; i<outer_size;i++){
// printf("leftIndexArray %d->%d and rightIndexArray %d ->%d\n",i,leftIndexArray[i],i,rightIndexArray[i]);
// }

  int temp=0;

  for(int i = 0; i<outer_size;i++){
    // printf("Entering inside the merging part with i->%d\n",i);
    int left_index = leftIndexArray[i];
    int right_index = rightIndexArray[i];
    //if left index and right index are same then we skip the iteration because there are no elements. 
    if (left_index==right_index){continue;}

    while ((temp<result_size) && (left_index<right_index)){
      // printf("Inside while for outer element index %d\n",i);
      outer_results[temp]=i;
      inner_results[temp]=left_index;
      // printf("adding values (%d,%d) in outer and inner result\n",outer_results[i],inner_results[i]);
      left_index+=1;
      temp+=1;
      
    }
//break if we reach the result size.
    if (temp==result_size){break;}
    // printf("Exiting merging part with i->%d\n",i);

  }

  // for (int i = 0; i<temp;i++){
  //   printf("(OuterResult,InnerResult)->(%ld,%ld)\n",outer_results[i],inner_results[i]);
  // }

  free(leftIndexArray);
  free(rightIndexArray);

  return temp;

}

int64_t band_join_simd(int64_t* inner, int64_t inner_size, int64_t* outer, int64_t outer_size, int64_t* inner_results, int64_t* outer_results, int64_t result_size, int64_t bound)
{
  /* In a band join we want matches within a range of values.  If p is the probe value from the outer table, then all
     reccords in the inner table with a key in the range [p-bound,p+bound] inclusive should be part of the result.

     Results are returned via two arrays. outer_results stores the index of the outer table row that matches, and
     inner_results stores the index of the inner table row that matches.  result_size tells you the size of the
     output array that has been allocated. You should make sure that you don't exceed this size.  If there are
     more results than can fit in the result arrays, then return early with just a prefix of the results in the result
     arrays. The return value of the function should be the number of output results.

     To do the binary search, you could use the low_bin_nb_simd you just implemented to search for the lower bounds in parallel

     Once you've found the lower bounds, do the following for each of the 4 search keys in turn:
        scan along the sorted inner array, generating outputs for each match, and making sure not to exceed the output array bounds.

     This inner scanning code does not have to use SIMD.
  */

 /*
  INITIAL CODE WHICH WAS GIVING INCONSISTENT RESULT. PLEASE SKIP TO END OF COMMENT FOR THE FINAL CODE.
 int size = 0;

  for(int i=0; (i<outer_size) && (size<result_size); i+=4){
    __m256i outer_vec = _mm256_loadu_si256((__m256i*)&outer[i]);
    __m256i lower_bound_vec = _mm256_sub_epi64(outer_vec, _mm256_set1_epi64x(bound));
    __m256i upper_bound_vec = _mm256_add_epi64(outer_vec, _mm256_set1_epi64x(bound));

    int64_t left_indexes[4];
    int64_t right_indexes[4];

    int remainder = outer_size - i;
    if (remainder >= 4) {

      __m256i left_result[1];
      __m256i right_result[1];

      low_bin_nb_simd(inner,inner_size,lower_bound_vec,left_result);
      low_bin_nb_simd(inner,inner_size,upper_bound_vec,right_result);

      _mm256_store_si256((__m256i*)left_indexes, left_result[0]);
      _mm256_store_si256((__m256i*)right_indexes, right_result[0]);

      for(int j = 0; j < 4; j++) {
        while((size < result_size) && (left_indexes[j] < right_indexes[j])) {
          outer_results[size] = i+j;
          inner_results[size] = left_indexes[j];
          left_indexes[j]++;
          size++;
        }
      }

    } else {

      if (remainder >= 1) {
        int64_t lower_bound = _mm256_extract_epi64(lower_bound_vec, 0);
        int64_t upper_bound = _mm256_extract_epi64(upper_bound_vec, 0);
        
        left_indexes[0] = low_bin_nb_mask(inner,inner_size,lower_bound);
        right_indexes[0] = low_bin_nb_mask(inner,inner_size,upper_bound);
      }

      if (remainder >= 2) {
        int64_t lower_bound = _mm256_extract_epi64(lower_bound_vec, 1);
        int64_t upper_bound = _mm256_extract_epi64(upper_bound_vec, 1);
        
        left_indexes[1] = low_bin_nb_mask(inner,inner_size,lower_bound);
        right_indexes[1] = low_bin_nb_mask(inner,inner_size,upper_bound);
      }

      if (remainder == 3) {
        int64_t lower_bound = _mm256_extract_epi64(lower_bound_vec, 2);
        int64_t upper_bound = _mm256_extract_epi64(upper_bound_vec, 2);
        
        left_indexes[2] = low_bin_nb_mask(inner,inner_size,lower_bound);
        right_indexes[2] = low_bin_nb_mask(inner,inner_size,upper_bound);
      }

      for(int j = 0; j < remainder; j++) {
        while((size < result_size) && (left_indexes[j] < right_indexes[j])) {
          outer_results[size] = i+j;
          inner_results[size] = left_indexes[j];
          left_indexes[j]++;
          size++;
        }
      }

    }

  }
  return size;
  */


 //Using similiar approach from the above function.
  
  // Create and allocate space to store left and right result indexes
  int64_t *leftIndexArray = malloc(outer_size * sizeof(int64_t));
  int64_t *rightIndexArray = malloc(outer_size * sizeof(int64_t));
  if (!leftIndexArray || !rightIndexArray) {
    free(leftIndexArray);
    free(rightIndexArray); 
    return -1; 
  }

  // Calculate multiplier and remainder to find number of simd function instances and individual mask function call instances
  int multiplier = outer_size/4;
  int remainder = outer_size%4;

  int temp_1=0;
  while (temp_1<multiplier){
    // compute lower and upper bound vectors using AVX functions
    __m256i outer_vec = _mm256_loadu_si256((__m256i*)&outer[4*temp_1]);
    __m256i lower_bound_vec = _mm256_sub_epi64(outer_vec, _mm256_set1_epi64x(bound));
    __m256i upper_bound_vec = _mm256_add_epi64(outer_vec, _mm256_set1_epi64x(bound));

    // Create temporary indexes to store each iteration's results
    int64_t left_indexes[4];
    int64_t right_indexes[4];

    // Call binary search function using simd
    low_bin_nb_simd(inner,inner_size,lower_bound_vec,(__m256i*) left_indexes);
    low_bin_nb_simd(inner,inner_size,upper_bound_vec,(__m256i*) right_indexes);

    // Store results in temporary arrays
    for (int i =0;i<4;i++){
      leftIndexArray[4*temp_1 + i]=left_indexes[i];
      rightIndexArray[4*temp_1 + i]=right_indexes[i];
    }
    temp_1+=1;

  }

  // Store indexes of remaining cases into the result arrays
  for(int i = 0; i<remainder;i++){
    leftIndexArray[4*temp_1 + i]=low_bin_nb_mask(inner, inner_size, outer[4*temp_1 + i]-bound);//array containing left index in the range.
    rightIndexArray[4*temp_1 + i]=low_bin_nb_mask(inner,inner_size,outer[4*temp_1 + i]+bound);//array containing right index of the range.
  }

  // temp will be the final result size
  int temp=0;

  for(int i = 0; i<outer_size;i++){
    int left_index = leftIndexArray[i];
    int right_index = rightIndexArray[i];
    
    if (left_index==right_index){continue;}

    // Store indexes into the result format
    while ((temp<result_size) && (left_index<right_index)){
      outer_results[temp]=i;
      inner_results[temp]=left_index;
      left_index+=1;
      temp+=1;
      
    }

    if (temp==result_size){break;}

  }

  // Clear index array space
  free(leftIndexArray);
  free(rightIndexArray);

  // Reuturn result size
  return temp;

}

int main(int argc, char *argv[])
{
	 long long counter;
	 int64_t arraysize, outer_size, result_size;
	 int64_t bound;
	 int64_t *data, *queries, *results;
	 int ret;
	 struct timeval before, after;
	 int repeats;
	 int64_t total_results;

	 // band-join arrays
	 int64_t *outer, *outer_results, *inner_results;


	 if (argc >= 5)
	   {
	     arraysize = atoi(argv[1]);
	     outer_size = atoi(argv[2]);
	     result_size = atoi(argv[3]);
	     bound = atoi(argv[4]);
	   }
	 else
	   {
	     fprintf(stderr, "Usage: db5242 inner_size outer_size result_size bound <repeats>\n");
	     exit(EXIT_FAILURE);
	   }

	 if (argc >= 6)
	   repeats = atoi(argv[5]);
	 else
	   {
	     repeats=1;
	   }
    //  printf("InsideMain and bound->%d\n",bound);

	 /* allocate the array and the queries for searching */
	 ret=posix_memalign((void**) &data,64,arraysize*sizeof(int64_t));
	 if (ret)
	 {
	   fprintf(stderr, "Memory allocation error.\n");
	   exit(EXIT_FAILURE);
	 }
	 ret=posix_memalign((void**) &queries,64,arraysize*sizeof(int64_t));
	 if (ret)
	 {
	   fprintf(stderr, "Memory allocation error.\n");
	   exit(EXIT_FAILURE);
	 }
	 ret=posix_memalign((void**) &results,64,arraysize*sizeof(int64_t));
	 if (ret)
	 {
	   fprintf(stderr, "Memory allocation error.\n");
	   exit(EXIT_FAILURE);
	 }

	 /* allocate the outer array and output arrays for band-join */
	 ret=posix_memalign((void**) &outer,64,outer_size*sizeof(int64_t));
	 if (ret)
	 {
	   fprintf(stderr, "Memory allocation error.\n");
	   exit(EXIT_FAILURE);
	 }
	 ret=posix_memalign((void**) &outer_results,64,result_size*sizeof(int64_t));
	 if (ret)
	 {
	   fprintf(stderr, "Memory allocation error.\n");
	   exit(EXIT_FAILURE);
	 }
	 ret=posix_memalign((void**) &inner_results,64,result_size*sizeof(int64_t));
	 if (ret)
	 {
	   fprintf(stderr, "Memory allocation error.\n");
	   exit(EXIT_FAILURE);
	 }


	   /* code to initialize data structures goes here so that it is not included in the timing measurement */
	   init(data,queries,arraysize);
	   band_init(outer,outer_size);

#ifdef DEBUG
	   /* show the arrays */
	   printf("data: ");
	   for(int64_t i=0;i<arraysize;i++) printf("%ld ",data[i]);
	   printf("\n");
	   printf("queries: ");
	   for(int64_t i=0;i<arraysize;i++) printf("%ld ",queries[i]);
	   printf("\n");
	   printf("outer: ");
	   for(int64_t i=0;i<outer_size;i++) printf("%ld ",outer[i]);
	   printf("\n");
#endif


	   /* now measure... */

	  //  gettimeofday(&before,NULL);

	  //  /* the code that you want to measure goes here; make a function call */
    //  printf("bulk_bin_search_start\n");
	  //  bulk_bin_search(data,arraysize,queries,arraysize,results, repeats);

	  //  gettimeofday(&after,NULL);
	  //  printf("Time in bulk_bin_search loop is %ld microseconds or %f microseconds per search\n", (after.tv_sec-before.tv_sec)*1000000+(after.tv_usec-before.tv_usec), 1.0*((after.tv_sec-before.tv_sec)*1000000+(after.tv_usec-before.tv_usec))/arraysize/repeats);



	  //  gettimeofday(&before,NULL);

	  //  /* the code that you want to measure goes here; make a function call */
    //  printf("bulk_bin_search_4x_start\n");
	  //  bulk_bin_search_4x(data,arraysize,queries,arraysize,results, repeats);

	  //  gettimeofday(&after,NULL);
	  //  printf("Time in bulk_bin_search_4x loop is %ld microseconds or %f microseconds per search\n", (after.tv_sec-before.tv_sec)*1000000+(after.tv_usec-before.tv_usec), 1.0*((after.tv_sec-before.tv_sec)*1000000+(after.tv_usec-before.tv_usec))/arraysize/repeats);
     
     
     

	   gettimeofday(&before,NULL);

	   /* the code that you want to measure goes here; make a function call */
	   total_results=band_join(data, arraysize, outer, outer_size, inner_results, outer_results, result_size, bound);

	   gettimeofday(&after,NULL);
	   printf("Band join result size is %ld with an average of %f matches per output record\n",total_results, 1.0*total_results/(1.0+outer_results[total_results-1]));
	   printf("Time in band_join loop is %ld microseconds or %f microseconds per outer record\n", (after.tv_sec-before.tv_sec)*1000000+(after.tv_usec-before.tv_usec), 1.0*((after.tv_sec-before.tv_sec)*1000000+(after.tv_usec-before.tv_usec))/outer_size);

#ifdef DEBUG
	   /* show the band_join results */
	   printf("band_join results: ");
	   for(int64_t i=0;i<total_results;i++) printf("(%ld,%ld) ",outer_results[i],inner_results[i]);
	   printf("\n");

#endif
    

    gettimeofday(&before,NULL);

	   /* the code that you want to measure goes here; make a function call */
	   total_results=band_join_simd(data, arraysize, outer, outer_size, inner_results, outer_results, result_size, bound);

	   gettimeofday(&after,NULL);
	   printf("Band join (SIMD) result size is %ld with an average of %f matches per output record\n",total_results, 1.0*total_results/(1.0+outer_results[total_results-1]));
	   printf("Time in band_join_simd loop is %ld microseconds or %f microseconds per outer record\n", (after.tv_sec-before.tv_sec)*1000000+(after.tv_usec-before.tv_usec), 1.0*((after.tv_sec-before.tv_sec)*1000000+(after.tv_usec-before.tv_usec))/outer_size);

#ifdef DEBUG
	   /* show the band_join results */
	   printf("band_join_simd results: ");
	   for(int64_t i=0;i<total_results;i++) printf("(%ld,%ld) ",outer_results[i],inner_results[i]);
	   printf("\n");
#endif


    // FILE *csvFile;
    // csvFile = fopen("band_join.csv", "w");
    // fprintf(csvFile, "inner, outer\n");
	  // for(int64_t i=0;i<total_results1;i++){
    //   fprintf(csvFile, "%ld, %ld\n", inner_results[i],outer_results[i]);
    // }
	  // fclose(csvFile);

	  // FILE *csvFile2;
    // csvFile2 = fopen("band_join_SIMD.csv", "w");
    // fprintf(csvFile2, "inner, outer\n");
	  // for(int64_t i=0;i<total_results2;i++){
    //   fprintf(csvFile2, "%ld, %ld\n", inner_results[i],outer_results[i]);
    // }
	  // fclose(csvFile2);

    // FILE *csvFile3;
    // csvFile3 = fopen("data.csv", "w");
    // fprintf(csvFile3, "inner, outer\n");
    // for(int64_t i=0;i<arraysize;i++){
    //   fprintf(csvFile3, "%ld, %ld\n", data[i], outer[i]);
    // }
    // fclose(csvFile3);


}

