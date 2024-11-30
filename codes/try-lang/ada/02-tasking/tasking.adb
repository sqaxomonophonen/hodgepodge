-- this pragma /has/ to be first because it's a "configuration pragma".
-- apparently it affects the entire compilation unit.
pragma Task_Dispatching_Policy (FIFO_Within_Priorities);

with Ada.Text_IO;
with System;
with Ada.Dynamic_Priorities;

procedure Tasking is
   task T0;
   task body T0 is
   begin
      -- trying to make some sched_setscheduler() "noise" in strace output:
      Ada.Dynamic_Priorities.Set_Priority(30);
      Ada.Dynamic_Priorities.Set_Priority(31);
      Ada.Dynamic_Priorities.Set_Priority(30);
      Ada.Dynamic_Priorities.Set_Priority(31);
      Ada.Dynamic_Priorities.Set_Priority(30);
      Ada.Dynamic_Priorities.Set_Priority(31);
      Ada.Dynamic_Priorities.Set_Priority(30);
      Ada.Dynamic_Priorities.Set_Priority(31);
      Ada.Dynamic_Priorities.Set_Priority(30);
      for I in 1..5 loop
         Ada.Text_IO.Put_Line ("hello from a task!" & Integer'Image(I));
         delay 0.222;
      end loop;
      Ada.Dynamic_Priorities.Set_Priority(40);
   end T0;
begin
   Ada.Text_IO.Put_Line("Hello.");
end Tasking;
