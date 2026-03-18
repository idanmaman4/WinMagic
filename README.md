# Cpp23 Warppers for Windbg 
<img width="1096" height="455" alt="image" src="https://github.com/user-attachments/assets/b14b9930-f86f-4875-8ea9-2330283b39ba" />
<img width="1223" height="543" alt="image" src="https://github.com/user-attachments/assets/69818735-0a52-41b4-af92-45091dcc4528" />




# Have you ever wondered why windbg is so slow: 

## when doing commands like !process 0 0 and !handle 0 0 it takes long time while alternative extension like swishDbgExt are super fast!
so let's dig into that
when I fire up 2 windbg and try !handle and I stop on the Ioctl method : 
https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdbgexts/nc-wdbgexts-pwindbg_ioctl_routine

We get 
<img width="2528" height="671" alt="image" src="https://github.com/user-attachments/assets/3040ac1d-251f-4d52-9839-493f6cd10f0e" />
<img width="2494" height="688" alt="image" src="https://github.com/user-attachments/assets/61c0bdf4-fdba-41aa-a9c1-6d039f846364" />
<img width="1029" height="602" alt="image" src="https://github.com/user-attachments/assets/9d45ca50-dfec-49c1-a205-1c7248eb4514" />

we can see the ioctl code is 0x16 which is 22 decimal which is IG_DUMP_SYMBOL_INFO
which means we are trying to resolve the same symbols again agian -> and internaly the dbgeng is doing a lot of work on each symbol so it's slow! 
which means if we implment even a cache per command run of the offsets we can spped windbg and make seemsless : 
in my pocs I dumped a process pid, name , peb-name and let's see how much time it takes in debug: 


So in this project I will be implmenting An efficent hooks to cache windbg internaly using com proxying to speed up windbg per command(exspoed through my extesion ) 

so for start I created a simple code in my cpp bindings for !process 0 0 like 

<img width="2103" height="1063" alt="image" src="https://github.com/user-attachments/assets/6c334cbd-12dc-4769-8fff-5b41ced340c2" />
3 secs for 183 processes 

and let's time the !process 0 0 

<img width="2537" height="1025" alt="image" src="https://github.com/user-attachments/assets/4320d821-e02f-4d61-9fc9-18bf73454c50" />


and now let's watch the proof in IDA for resolving symbols in a loop in an inefficent way:
<img width="1780" height="93" alt="image" src="https://github.com/user-attachments/assets/328df96e-df1c-4c49-aa83-d5dd4f3ce692" />
<img width="1597" height="102" alt="image" src="https://github.com/user-attachments/assets/28d18871-f18b-430f-acd7-1c4bf8aa4f01" />
<img width="1840" height="838" alt="image" src="https://github.com/user-attachments/assets/4576a61e-8c7a-4500-9711-f51ea96a1bbe" />
<img width="1800" height="394" alt="image" src="https://github.com/user-attachments/assets/b61d6d92-2000-41ac-856d-c182ca81cdd7" />
<img width="1648" height="586" alt="image" src="https://github.com/user-attachments/assets/a0b85e32-d50f-45c9-8d0d-2867fd9729da" />



and in SwishDbg: 
<img width="1113" height="850" alt="image" src="https://github.com/user-attachments/assets/2c2dc918-59b2-4c54-b377-83a2f39346f7" />


