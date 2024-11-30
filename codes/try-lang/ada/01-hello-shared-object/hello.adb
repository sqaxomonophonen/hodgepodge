with Ada.Text_IO;
package body Hello is

   task type My_Task_Type;

   task body My_Task_Type is
   begin
      for I in 1..10 loop
         Ada.Text_IO.Put_Line ("hello from a task!" & Integer'Image(I));
         delay 0.1;
      end loop;
   end My_Task_Type;

   procedure Hello_World is
      My_Task : access My_Task_Type;
   begin
      My_Task := new My_Task_Type;
      Ada.Text_IO.Put_Line ("hellø wørld!");
   end Hello_World;
end Hello;
