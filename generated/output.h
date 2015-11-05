void _ZN14EchoIndication4echoEi(unsigned int Vv);
bool _ZN5Fifo1IiE8deq__RDYEv(class l_class_OC_Fifo1 *Vthis);
bool _ZN5Fifo1IiE10first__RDYEv(class l_class_OC_Fifo1 *Vthis);
void _ZN5Fifo1IiE3deqEv(class l_class_OC_Fifo1 *Vthis);
unsigned int _ZN5Fifo1IiE5firstEv(class l_class_OC_Fifo1 *Vthis);
void _ZN14EchoIndication4echoEi(unsigned int Vv);
bool _ZN5Fifo1IiE8enq__RDYEv(class l_class_OC_Fifo1 *Vthis);
void _ZN5Fifo1IiE3enqEi(class l_class_OC_Fifo1 *Vthis, unsigned int Vv);
class l_class_OC_Fifo1;
class l_class_OC_Fifo;
class l_class_OC_Module;
class l_class_OC_Module {
public:
  class l_class_OC_Rule *rfirst;
  class l_class_OC_Module *next;
  class l_class_OC_Module *shadow;
  unsigned long long size;
};

class l_class_OC_Fifo {
public:
  unsigned int  (**) ( int, ...);
  class l_class_OC_Module _vptr_EC_Fifo;
};

class l_class_OC_Fifo1 {
public:
  class l_class_OC_Fifo ;
  unsigned int element;
  bool full;
  bool field3[3];
  bool enq__RDY(void) {
        return ((full) ^ 1);
  }
  void enq(unsigned int v) {
        (element) = v;
        (full) = 1;
  }
  bool deq__RDY(void) {
        return (full);
  }
  void deq(void) {
        (full) = 0;
  }
  bool first__RDY(void) {
        return (full);
  }
  unsigned int first(void) {
        return (element);
  }
};

class l_class_OC_EchoTest;
class l_class_OC_EchoTest_KD__KD_drive;
class l_class_OC_Rule;
class l_class_OC_Rule {
public:
  unsigned int  (**_vptr_EC_Rule) ( int, ...);
  class l_class_OC_Rule *next;
};

class l_class_OC_EchoTest_KD__KD_drive {
public:
  class l_class_OC_Rule ;
  class l_class_OC_EchoTest *module;
  bool RDY(void) {
    bool tmp__1 =     ((*(((module)->echo)->fifo))[2])();
        return tmp__1;
  }
  void ENA(void) {
        ((*(((module)->echo)->fifo))[3])(22);
  }
};

class l_class_OC_EchoTest {
public:
  class l_class_OC_Module ;
  class l_class_OC_Echo *echo;
  unsigned int x;
  class l_class_OC_EchoTest_KD__KD_drive driveRule;
};

class l_class_OC_Echo_KD__KD_respond_KD__KD_respond2;
class l_class_OC_Echo_KD__KD_respond_KD__KD_respond2 {
public:
  class l_class_OC_Rule ;
  class l_class_OC_Echo *module;
  void ENA(void) {
  }
  bool RDY(void) {
        return 1;
  }
};

class l_class_OC_Echo_KD__KD_respond_KD__KD_respond1;
class l_class_OC_Echo_KD__KD_respond_KD__KD_respond1 {
public:
  class l_class_OC_Rule ;
  class l_class_OC_Echo *module;
  bool RDY(void) {
    bool tmp__1 =     ((*((module)->fifo))[4])();
    bool tmp__2 =     ((*((module)->fifo))[6])();
        return (tmp__1 & tmp__2);
  }
  void ENA(void) {
        ((*((module)->fifo))[5])();
    unsigned int call =     ((*((module)->fifo))[7])();
        _ZN14EchoIndication4echoEi();
  }
};

