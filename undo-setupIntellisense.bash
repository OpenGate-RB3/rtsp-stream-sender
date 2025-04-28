set -e

if [[ -z "$1" ]]; then
    echo "must set path to sdk root: ${1}";
    exit 1
fi

git update-index --no-assume-unchanged ./.vscode/c_cpp_properties.json

source $1/environment-setup-armv8-2a-qcom-linux

sed -i "s#${SDKTARGETSYSROOT}#\${command:qvsce.getProjectSdkToolchainPath}#g" ./.vscode/c_cpp_properties.json