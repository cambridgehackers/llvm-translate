class l_class_OC_Fifo {
public:
  unsigned long long unused_data_to_force_inheritance;
};

class l_class_OC_Fifo1 {
public:
  unsigned long long unused_data_to_force_inheritance;
  unsigned int element;
  bool full;
  bool enq__RDY(void);
  void enq(unsigned int Vv);
  bool deq__RDY(void);
  void deq(void);
  bool first__RDY(void);
  unsigned int first(void);
};

class l_class_OC_Echo {
public:
  unsigned long long unused_data_to_force_inheritance;
  class l_class_OC_Fifo1 fifo;
  class l_class_OC_EchoIndication *ind;
  unsigned int pipetemp;
  bool rule_respond__RDY(void);
  void rule_respond(void);
  void run();
};

class l_class_OC_EchoTest {
public:
  unsigned long long unused_data_to_force_inheritance;
  class l_class_OC_Echo *echo;
  unsigned int x;
  void rule_drive(void);
  bool rule_drive__RDY(void);
  void run();
};

