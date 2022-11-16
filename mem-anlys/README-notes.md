Memory Management:
  Finding un used  variable for each class:
    Window:
      fload ws , zs , multiplierAvg   DONE
      int number_of_samples           DONE
    AccessTime:
    Address:
      int rDist                       DONE
    Instruction:
    CPU:
    Function:

  Maybe change:
    maybe move funcName from Address class to instruction calls

  //OZGURCLEANUP

  enum EnumType : uint16_t
  {
    Bar,
    Baz,
    Bork,
  };

      - Savings/entry      
  - 12 pointers: 4 objects x 3 pointers      
  - 2 bytes: 'cpuid' is short      
  - 4 bytes: 'int rDist' is not needed      
  - ~50-100/bytes: string for func-name; should be a id in string table 
  - ~100/bytes: string for load module; should be a id in string table 
  - 2 bytes: load class is a short      
  - 2 bytes: extra-frame-loads is a short

  AccessTime                               
      u long + int 
  CPU -> short id  instead of int  2 bytes ->  only 2 bytes
  instruction
      type should be ENUM ==>>
      extra constant load  -> short                                   
  Fname and DSO(load_module) names should be a string table or hash map 
  Address: rm fName 


In MAIN I started to do modificcation:

are theese necessary ??
  299   vector <unsigned long> Zs;
   300   vector <unsigned long> Zt;
    301   vector <unsigned long> wSize;
     302   vector <unsigned long> wTime;
      303   vector <double> wMultipliers;


WINDOW.HPP:

62     Address* maxFP_addr; //TODO Why we need this??
