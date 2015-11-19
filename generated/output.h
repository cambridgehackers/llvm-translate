class l_class_OC_EchoTest {
public:
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
};

class l_class_OC_Fifo1 {
public:
  unsigned int element;
  bool full;
  void deq(void);
  bool enq__RDY(void);
  void enq(unsigned int Vv);
  bool deq__RDY(void);
  bool first__RDY(void);
  unsigned int first(void);
};

