// Serial Chat

main() {
   int ret;

   clear();
   puts("Serial Chat\n");
   ret = seropen(57600, "8N1C", 50);
   puts("seropen() - " + ret + "\n");
   while (1) {
      if (event(0)==1)
         // grafitti written
         sersend(key());
      if (serdata())
         // data waiting
         puts((char)serrecv());
      // save batteries
      sleep(10);
   }
   // This line included for completeness
   serclose();
}
