const { exec } = require('child_process');
const path = require('path');
const pkg = require('./package.json');

const version = pkg.version;
const exePath = path.resolve(__dirname, '../Build/Service/ufserverservice-win.exe');
const iconPath = path.resolve(__dirname, 'public/icon.ico');

// Path to your rcedit executable (update if needed)
const rceditExe = 'D:/Github/DayZ-UniveralApi/Build/rcedit.exe';

// Build the rcedit command with dynamic version numbers:
const rceditCmd = `"${rceditExe}" "${exePath}" --set-icon "${iconPath}" --set-requested-execution-level requireAdministrator --set-file-version "${version}" --set-product-version "${version}" --set-version-string "ProductName" "Universal Framework Service" --set-version-string "CompanyName" "Daemonforge Developments" --set-version-string "FileDescription" "Universal Framework Service, https://daemonforge.dev/" --set-version-string "OriginalFilename" "ufserverservice-win.exe"`;

console.log('Running rcedit:', rceditCmd);
exec(rceditCmd, (err, stdout, stderr) => {
  if (err) {
    console.error('Error running rcedit:', stderr);
    process.exit(1);
  }
  console.log('rcedit output:', stdout);
});
