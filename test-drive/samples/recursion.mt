// Matte supports recursion in any context.
// 
@:fibonacci ::(n) {
   return match(true) {
     (n <  1): 0,
     (n <= 2): 1,
     default: fibonacci(n:n-1) + fibonacci(n:n-2)
   };
};
  
[0, 7]->for(do:::(i) {
   print(message:fibonacci(n:i));
});
