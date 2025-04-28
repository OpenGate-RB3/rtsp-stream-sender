set -e # crash and stop

if [[ -z "$1" ]]; then
    echo "must set path to sdk root: ${1}";
    exit 1
fi

git update-index --assume-unchanged ./.vscode/c_cpp_properties.json

source $1/environment-setup-armv8-2a-qcom-linux # needed since we need some vars

sed -i "s#\${command:qvsce.getProjectSdkToolchainPath}#${SDKTARGETSYSROOT}#g" ./.vscode/c_cpp_properties.json
