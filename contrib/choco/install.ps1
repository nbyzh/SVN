param($installPath, $toolsPath, $package, $project)

Write-Host 'This is not a package with dlls to reference in it.'
Write-Host 'Please install using chocolatey'
Write-Host "To get chocolatey just run 'Install-Package chocolatey' followed by 'Initialize-Chocolatey'"
Write-Host 'chocolatey install tortoisesvn'
write-host 'Removing this package...'
uninstall-package tortoisesvn -ProjectName $project.Name$project.Name
