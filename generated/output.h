void _ZN14EchoIndication4echoEi(unsigned int Vv);
bool _ZN5Fifo1IiE8deq__RDYEv(class l_class_OC_Fifo1 *Vthis);
bool _ZN5Fifo1IiE10first__RDYEv(class l_class_OC_Fifo1 *Vthis);
void _ZN5Fifo1IiE3deqEv(class l_class_OC_Fifo1 *Vthis);
unsigned int _ZN5Fifo1IiE5firstEv(class l_class_OC_Fifo1 *Vthis);
void _ZN14EchoIndication4echoEi(unsigned int Vv);
bool _ZN5Fifo1IiE8enq__RDYEv(class l_class_OC_Fifo1 *Vthis);
void _ZN5Fifo1IiE3enqEi(class l_class_OC_Fifo1 *Vthis, unsigned int Vv);
class l_class_OC_Module {
public:
  class l_class_OC_Rule *rfirst;
  class l_class_OC_Echo *next;
  unsigned long long size;
};

class l_class_OC_EchoTest {
public:
  class l_class_OC_Rule *rfirst;
  class l_class_OC_Echo *next;
  unsigned long long size;
  class l_class_OC_Echo *echo;
  unsigned int x;
  bool rule_drive__RDY(void);
  void rule_drive(void);
};

class l_class_OC_EchoIndication {
public:
};

class l_class_OC_Echo {
public:
  class l_class_OC_Rule *rfirst;
  class l_class_OC_Echo *next;
  unsigned long long size;
  class l_class_OC_Fifo1 *fifo;
  class l_class_OC_EchoIndication *ind;
  unsigned int pipetemp;
  bool rule_respond__RDY(void);
  void rule_respond(void);
};

class l_class_OC_Fifo {
public:
};

class l_class_OC_Fifo1 {
public:
  unsigned int element;
  bool full;
  bool deq__RDY(void);
  bool enq__RDY(void);
  void enq(unsigned int Vv);
  void deq(void);
  bool first__RDY(void);
  unsigned int first(void);
};

class l_class_OC_Rule {
public:
};

