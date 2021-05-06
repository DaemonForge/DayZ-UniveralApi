
module.exports ={
    dynamicSortMultiple,
    dynamicSort,
    isObject,
    isArray,
    isEmpty,
    makeAuthToken,
    makeObjectId,
    RemoveBadProperties,
    versionCompare
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

function isEmpty(obj){
    return (obj && Object.keys(obj).length === 0 && obj.constructor === Object)
}

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


 function RemoveBadProperties(obj){
    let replace = /[\!\@\#\$\%\^\&\*\(\)\+\=\\\|\]\[\"\?\>\<\.\,\;\:\- ]/g;
    Object.keys(obj).forEach(function (k) {
        if (isObject(obj[k])) {
            obj[k] = RemoveBadProperties(obj[k]);
            return;
        }
        if (isArray(obj[k])){
            obj[k].forEach(j =>{
                if (isObject(j)){
                    j = RemoveBadProperties(j);
                }
            });
        }
        let K = k.replace(replace, "_");
        if (K !== k){
            obj[K] = obj[k];
            delete obj[k];
        }
    })
    return obj;
}


function versionCompare(v1, v2, options) {
    var lexicographical = options && options.lexicographical,
        zeroExtend = options && options.zeroExtend,
        v1parts = v1.split('.'),
        v2parts = v2.split('.');

    function isValidPart(x) {
        return (lexicographical ? /^\d+[A-Za-z]*$/ : /^\d+$/).test(x);
    }

    if (!v1parts.every(isValidPart) || !v2parts.every(isValidPart)) {
        return NaN;
    }

    if (zeroExtend) {
        while (v1parts.length < v2parts.length) v1parts.push("0");
        while (v2parts.length < v1parts.length) v2parts.push("0");
    }

    if (!lexicographical) {
        v1parts = v1parts.map(Number);
        v2parts = v2parts.map(Number);
    }

    for (var i = 0; i < v1parts.length; ++i) {
        if (v2parts.length == i) {
            return 1;
        }

        if (v1parts[i] == v2parts[i]) {
            continue;
        }
        else if (v1parts[i] > v2parts[i]) {
            return 1;
        }
        else {
            return -1;
        }
    }

    if (v1parts.length != v2parts.length) {
        return -1;
    }

    return 0;
}