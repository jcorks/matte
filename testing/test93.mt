return 0;
@class = import(module:'Matte.Class');

@Expensive = class(definition:{
    instantiate::(this, thisType) {
        @revivalCount = 0;
        @accum = 'a';
        this.interface = {
            use ::{
                accum = accum + 'a';
            },
            
            onRevive ::{
                print(message:"During this lifetime, ive accumulated " + accum + '\n');
                accum = 'a';
                revivalCount+=1;
            }
            
        }
        
        
        this.operator = {
            (String) :: {
                return 'IAM' + accum;
            }        
        }
    }
});


::<={
    ::<= {
        @a = Expensive.new();
        @b = Expensive.new();
        @c = Expensive.new();
        @d = Expensive.new();


        a.use();
            b.use();
            d.use();
        
    }
}
::<={}
::<={}

::<= {
    @a = Expensive.new();
    @b = Expensive.new();
    @c = Expensive.new();
    @d = Expensive.new();


    a.use();
    for(0, 10)::{
        b.use();
        d.use();
    }
    
}




return 'a';

