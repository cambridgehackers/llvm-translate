class l_class_OC_EchoTest {
public:
  class l_class_OC_Rule *rfirst;
  class l_class_OC_Echo *next;
  unsigned long long size;
  class l_class_OC_Echo *echo;
  unsigned int x;
  bool rule_drive__RDY(void);
  void rule_drive(void);
  void run();
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
  void run();
};

class l_class_OC_Fifo {
public:
  class l_class_OC_Rule *rfirst;
  class l_class_OC_Echo *next;
  unsigned long long size;
};

class l_class_OC_Fifo1 {
public:
  class l_class_OC_Rule *rfirst;
  class l_class_OC_Echo *next;
  unsigned long long size;
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
  class l_class_OC_Rule *next;
};

