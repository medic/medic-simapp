
var fs = require('fs'),
    jsdump = require('jsDump');

if (!process.argv[2]) {
    console.log('Usage: node json-formatter.js file');
    process.exit();
}

fs.readFile(process.argv[2], function (err, data) {
    console.log(
        jsdump.parse(
            JSON.parse(data.toString())
        )
    );
});

