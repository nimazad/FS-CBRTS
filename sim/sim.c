#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>

#define NO_NODS			5
#define NO_INPUT		3
#define Learning_rate		20



struct ANN
{
  int U[NO_NODS];
  int W[NO_INPUT][NO_NODS];
};

static inline unsigned long activation_function(unsigned long server_period, unsigned long input)
{
     if(input <= 0)
	return 0;
     if(input > server_period*100)
       return server_period*100;
    return input;
}
static inline void teach_network(struct ANN net, unsigned long budget[NO_INPUT], unsigned long estimated_budget, unsigned long real_budget)
{
    long out_err, hidden[NO_NODS], hidden_err[NO_NODS];
    int input, node;
    
    out_err = real_budget - estimated_budget;
    printf("error  %ld\n", out_err);  
    
    for(node=0;node<NO_NODS;node++)
    {
      hidden[node] = 0;
      for(input=0;input<NO_INPUT;input++)
      {
  	  hidden[node] +=  activation_function(5, net.W[input][node]*budget[input]);
      }
      hidden_err[node] = 0;
      hidden_err[node] = out_err * hidden[node];
      net.U[node] += out_err * Learning_rate / 100;
      
    }
    for(node=0;node<NO_NODS;node++)
    {
      for(input=0;input<NO_INPUT;input++)
      {
  	  net.W[input][node] += hidden_err[node] * Learning_rate / 100 /100;
      }
    }
}

static inline unsigned long ask_network(struct ANN net, unsigned long budget[NO_INPUT])
{
  int input, node;
  unsigned long hidden[NO_NODS], output;

  for(node=0;node<NO_NODS;node++)
    hidden[node] = 0;
  output = 0;
  
//   print_warning("ask_network", "");
  for(node=0;node<NO_NODS;node++)
  {
    for(input=0;input<NO_INPUT;input++)
    {
	hidden[node] +=  activation_function(5, net.W[input][node]*budget[input]);
    }
  }
  
  for(node=0;node<NO_NODS;node++)
  {
      output += activation_function(5, net.U[node]*hidden[node]/100 + (0 ? net.U[node]*hidden[node]% 100 == 0 : 1));
  }
  
  output = output / 100 + (0 ? output % 100 == 0 : 1);
	      
//   print_time("ask_network","network output", output);
   printf("Nima-HSF ==> in function network output %lu.\n", output);
  return output;
}



static inline void my_swap(void* array_add , int first, int second)
{
	unsigned long *array = array_add;
	unsigned long temp;
	temp = array[first];
	array[first] = array[second];
	array[second] = temp;
}
static inline int partition(void* array_add ,int left, int right) 
{
	int storeIndex = left;
	int piv_index;
	int pivot, i;
	unsigned long *array = array_add;
	//choose the pivot
	piv_index = (left + right)/2;
	pivot = array[piv_index];
	my_swap(array, piv_index, right);
	
	for(i=left;i<right;i++)
	{
		if(array[i] < pivot)
		{
			my_swap(array, i, storeIndex);
			storeIndex = storeIndex + 1;
		}
	}
	my_swap(array, storeIndex, right);
	//return pivots position
	return storeIndex;
}

static inline void quicksort(void* array_add , int l, int r)
{
	int pivot;
	if(r>l)
	{
		pivot = partition(array_add, l, r);
		quicksort(array_add, l, pivot-1);
		quicksort(array_add, pivot+1, r);
	}
}

int main(int argc, char* argv[])
{
	int i, j;
	unsigned long ANN_input[NO_INPUT];
	struct ANN my_net;
//	ANN_input = malloc(sizeof(unsigned long)*NO_INPUT);
//printf("system clock %ld\n", sysconf(_SC_CLK_TCK));
	/*
	 * initializing ANN
	 */
	for(i=0;i<NO_NODS;i++)
	  my_net.U[i] = 100/NO_NODS;
	
	for(i=0;i<NO_INPUT;i++)
	  for(j=0;j<NO_NODS;j++)
	      my_net.W[i][j] = 100/NO_INPUT;
	
	for(i=0;i<NO_INPUT;i++)
		ANN_input[i] = i+1;
ANN_input[0] = 5;
ANN_input[1] = 3;
ANN_input[2] = 2;
//	teach_network(my_net, ANN_input, 	ask_network(my_net, ANN_input), 2);
//	ask_network(my_net, ANN_input);
	for(i=0;i<NO_INPUT;i++)
		printf("%lu\n", ANN_input[i]);
	quicksort((void *)&ANN_input, 0, NO_INPUT-1);
	for(i=0;i<NO_INPUT;i++)
		printf("%lu\n", ANN_input[i]);

	return 0;
}

