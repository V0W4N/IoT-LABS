// main.cpp


// Select current lab to compile:
// Uncomment the desired line and comment out the others


// #define LAB_TEST
// #define LAB_0
// #define LAB_1_1
// #define LAB_1_2
// #define LAB_2_1
// #define LAB_2_2
// #define LAB_3_1
 #define LAB_3_2
// #define LAB_4_1
// #define LAB_4_2
// #define LAB_5_1
// #define LAB_5_2
// #define LAB_6_1
// #define LAB_6_2
// #define LAB_7_1
// #define LAB_7_2
// #define LAB_7_3

#ifdef LAB_TEST
  #include "labs/test.cpp"
#elif defined(LAB_0)
  #include "labs/Lab0.cpp"
#elif defined(LAB_1_1)
  #include "labs/Lab1_1.cpp"
#elif defined(LAB_1_2)
  #include "labs/Lab1_2.cpp"
#elif defined(LAB_2_1)
  #include "labs/Lab2_1.cpp"
#elif defined(LAB_2_2)
  #include "labs/Lab2_2.cpp"
#elif defined(LAB_3_1)
  #include "labs/Lab3_1.cpp"
#elif defined(LAB_3_2)
  #include "labs/Lab3_2.cpp"
#elif defined(LAB_4_1)
  #include "labs/Lab4_1.cpp"
#elif defined(LAB_4_2)
  #include "labs/Lab4_2.cpp"
#elif defined(LAB_5_1)
  #include "labs/Lab5_1.cpp"
#elif defined(LAB_5_2)
  #include "labs/Lab5_2.cpp"
#elif defined(LAB_6_1)
  #include "labs/Lab6_1.cpp"
#elif defined(LAB_6_2)
  #include "labs/Lab6_2.cpp"
#elif defined(LAB_7_1)
  #include "labs/Lab7_1.cpp"
#elif defined(LAB_7_2)
  #include "labs/Lab7_2.cpp"
#elif defined(LAB_7_3)
  #include "labs/Lab7_3.cpp"
#else
  // ERROR_LAB runs by default
  #define ERROR_LAB
  #include "labs/ErrorLab.cpp"
#endif