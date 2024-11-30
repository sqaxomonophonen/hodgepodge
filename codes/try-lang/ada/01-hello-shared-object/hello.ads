package Hello is
   procedure Hello_World
      with
         Export => True,
         Convention => C,
         External_Name => "hello_world";
end Hello;
