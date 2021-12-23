
::<={

    @preserveMe = {};

    // Now that the object has a preserver, 
    // this function will run exactly once.
    setAttributes(
        of        : preserveMe,
        attributes: {
            'preserver' ::{
                print(message:"I've been preserved!");                
            }
        }
    );
};

// Since preserveMe is within an anonymous dashed function
// and no reachable variable references it,
// it is no longer considered active. The runtime 
// may decide at any time to run the preserver. 
print(message:2+2);


// After the returning of this, and assuming that this 
// source consists of the entire Matte program, 
// the preserver MUST be called by the runtime after the 
// termination of the program.
//
// However, if it was already run, then the object 
// would not have its preserver run again since it 
// is not set once more.
return 6+8;