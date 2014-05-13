TARGET	:= libhsf
INCDIR	:= /usr/src/kernels/$(shell uname --release)/include

$(TARGET).o:
	@gcc -o $(TARGET).o -c $(TARGET).c
	@ar -rcs $(TARGET).a $(TARGET).o
	@rm -f $(TARGET).o

clean:
	@rm -f $(TARGET).o $(TARGET).a *~
