//// Test 125
//
//  setModule 

@out = "";
::<= {
  setModule(
    name: 'Test.Module',
    value ::<= {
      @accum = 0;
      
      return {
        add :: {
          accum+=1;
        },
        
        say ::<- accum,
        
        reset ::<- accum = 0
      }
    }
  )
  
  ::? {
    setModule(name:0, value:0);
  } => {
    onError::(message) <- out = out + 'err'
  }

  ::? {
    setModule(name:'Matte.Core.Class', value:0);
  } => {
    onError::(message) <- out = out + 'err'
  }
  
  ::? {
    setModule(name:'Test.Module', value:{});
  } => {
    onError::(message) <- out = out + 'err'
  }
  

}



@:wahoo = import(:'Test.Module');

wahoo.add();
wahoo.add();
wahoo.add();
wahoo.add();
out = out + wahoo.say();
wahoo.reset();
out = out + wahoo.say();
return out;
