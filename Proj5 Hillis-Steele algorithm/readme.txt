In order to implement the Hillis-Steele algorithm, we used the idea of an old and a new array in which the new array gets filled up and is then copied to the old array. The neighbors (amount of numbers to be copied to the new array without any calculations) is determined by doing 2^i power where i is the step (or row). If the current column is greater than or equal to neighbors, then the current value plus the current value - neighbors is added to the new array.

Threads were mostly used for computing the values to put in the new array. The old array was divided up into sections equal to the amount of threads (i.e. 2 sections for 2 threads). We used an index tied to each thread which gave the starting position of the section for that thread. Then, the thread looped through the array and did its job.

Throughout the project we employed many locks in order to ensure that no two threads tried to update the same value at the same time. This was especially important when implementing a condition feature in order to make sure that all threads were done before moving down a row.

Our design maximizes opportunities for concurrency by splitting up the calculations between threads. Rather than have only one thread do all the calculations, many threads can work together in order to create the new array with the correct values.

This project was brought to you by Nat and Owen!  Have a great break!
(nmrubin, ocpfannenstiehl)
