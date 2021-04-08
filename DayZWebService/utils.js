
module.exports ={
    dynamicSortMultiple,
    dynamicSort,
    isObject,
    isArray,
    makeAuthToken,
    makeObjectId
}




function dynamicSortMultiple( props ) {
    /*
     * save the arguments object as it will be overwritten
     * note that arguments object is an array-like object
     * consisting of the names of the properties to sort by
     */
    return function (obj1, obj2) {
        var i = 0, result = 0, numberOfProperties = props.length;
        /* try getting a different result from 0 (equal)
         * as long as we have extra properties to compare
         */
        while(result === 0 && i < numberOfProperties) {
            result = dynamicSort(props[i])(obj1, obj2);
            i++;
        }
        return result;
    }
}
function dynamicSort(property) {
    var sortOrder = 1;
    if(property[0] === "-") {
        sortOrder = -1;
        property = property.substr(1);
    }
    return function (a,b) {
        /* next line works with strings and numbers, 
         * and you may want to customize it to your needs
         */
        var result = (a[property] < b[property]) ? -1 : (a[property] > b[property]) ? 1 : 0;
        return result * sortOrder;
    }
}

function isObject(a) {
    return (!!a) && (a.constructor === Object);
};


function isArray(a) {
    return (!!a) && (a.constructor === Array);
};

function makeAuthToken() {
    let result           = '';
    let characters       = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-.!~';
    let charactersLength = characters.length;
    for ( let i = 0; i < 48; i++ ) {
       result += characters.charAt(Math.floor(Math.random() * charactersLength));
    }
    return result;
 }

 function makeObjectId() {
    let result           = '';
    let characters       = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-.~()*:@,;';
    let charactersLength = characters.length;
    for ( let i = 0; i < 16; i++ ) {
       result += characters.charAt(Math.floor(Math.random() * charactersLength));
    }
    let datetime = new Date();
    let date = datetime.toISOString()
    result += date;
    let SaveToken = createHash('sha256').update(result).digest('base64');
    //Making it URLSafe
    SaveToken = SaveToken.replace(/\+/g, '-'); 
    SaveToken = SaveToken.replace(/\//g, '_');
    SaveToken = SaveToken.replace(/=+$/, '');
    return SaveToken;
 }