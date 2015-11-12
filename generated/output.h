void _ZN14EchoIndication4echoEi(unsigned int Vv);
bool _ZN5Fifo1IiE8deq__RDYEv(class l_class_OC_Fifo1 *Vthis);
bool _ZN5Fifo1IiE10first__RDYEv(class l_class_OC_Fifo1 *Vthis);
void _ZN5Fifo1IiE3deqEv(class l_class_OC_Fifo1 *Vthis);
unsigned int _ZN5Fifo1IiE5firstEv(class l_class_OC_Fifo1 *Vthis);
void _ZN14EchoIndication4echoEi(unsigned int Vv);
bool _ZN5Fifo1IiE8enq__RDYEv(class l_class_OC_Fifo1 *Vthis);
void _ZN5Fifo1IiE3enqEi(class l_class_OC_Fifo1 *Vthis, unsigned int Vv);
class l_class_OC_Rule {
public:
  class l_class_OC_Rule *next;
};

class l_class_OC_Echo_KD__KD_respond_KD__KD_respond2 {
public:
  class l_class_OC_Rule *next;
  class l_class_OC_Echo *module;
  void ENA(void);
  bool RDY(void);
};

class l_class_OC_Echo_KD__KD_respond_KD__KD_respond1 {
public:
  class l_class_OC_Rule *next;
  class l_class_OC_Echo *module;
  bool RDY(void);
  void ENA(void);
};

class l_class_OC_EchoTest_KD__KD_drive {
public:
  class l_class_OC_Rule *next;
  class l_class_OC_EchoTest *module;
  bool RDY(void);
  void ENA(void);
};

class l_class_OC_Module {
public:
  class l_class_OC_Rule *rfirst;
  class l_class_OC_Module *next;
  unsigned long long size;
};

class l_class_OC_Fifo {
public:
// generateClassElements: inherit failed
};

class l_class_OC_Fifo1 {
public:
// generateClassElements: inherit failed
  unsigned int element;
  bool full;
  bool enq__RDY(void);
  void enq(unsigned int Vv);
  bool deq__RDY(void);
  void deq(void);
  bool first__RDY(void);
  unsigned int first(void);
};

class l_class_OC_EchoTest {
public:
  class l_class_OC_Rule *rfirst;
  class l_class_OC_Module *next;
  unsigned long long size;
  class l_class_OC_Echo *echo;
  unsigned int x;
  class l_class_OC_EchoTest_KD__KD_drive driveRule;
};

class l_class_OC_Echo_KD__KD_respond {
public:
  class l_class_OC_Echo_KD__KD_respond_KD__KD_respond1 respond1Rule;
  class l_class_OC_Echo_KD__KD_respond_KD__KD_respond2 respond2Rule;
};

class l_class_OC_Echo {
public:
  class l_class_OC_Rule *rfirst;
  class l_class_OC_Module *next;
  unsigned long long size;
  class l_class_OC_Fifo1 *fifo;
  class l_class_OC_EchoIndication *ind;
  unsigned int pipetemp;
  class l_class_OC_Echo_KD__KD_respond respondRule;
};

class l_class_OC_EchoIndication {
public:
};

