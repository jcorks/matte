// Matte supports recursion in any context.
// 
@:fibonacci ::(n) {
   return match(true) {
     (n <  1): 0,
     (n <= 2): 1,
     default: fibonacci(n:n-1) + fibonacci(n:n-2)
   }
}
  
for(0, 7)::(i) {
   print(message:fibonacci(n:i));
}
