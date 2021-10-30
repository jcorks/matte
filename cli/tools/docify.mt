@System = import('system/system.mt');



@getDirectoryList = ::{
    @arr = System.directoryContents;
    @out = new Array();
    
    foreach(arr, ::(key, val) {
        
        System.println(val.path);
    });

}


return System;
