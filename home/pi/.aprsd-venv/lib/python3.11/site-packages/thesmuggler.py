import inspect
import os
import sys

__PYTHON_BELOW_33 = sys.version_info < (3, 4)
__PYTHON_33_34 = (sys.version_info > (3, 3)) and (sys.version_info < (3, 5))
__PYTHON_35_PLUS = sys.version_info > (3, 5)

if __PYTHON_BELOW_33:
    import imp
if __PYTHON_33_34:
    from importlib.machinery import SourceFileLoader
if __PYTHON_35_PLUS:
    import importlib.util


def smuggle(*args, **kwargs):
    # type: (*str, **str) -> Any

    """
    Returns the provided soure file as a module.

    Usage:

        weapons = smuggle('weapons.py')
        drugs, alcohol = smuggle('drugs', 'alcohol', source='contraband.py')
    """
    source = kwargs.pop('source', None)

    # Be careful when moving the contents of this
    module_file = args[0] if len(args) == 1 else source

    module_name = os.path.splitext(os.path.basename(module_file))[0]

    if os.path.isabs(module_file):
        abs_path = module_file
    else:
        directory = os.path.dirname(inspect.stack()[1][1])
        abs_path = os.path.normpath(os.path.join(directory, module_file))

    if __PYTHON_BELOW_33:
        module = imp.load_source(module_name, abs_path)

    if __PYTHON_33_34:
        module = SourceFileLoader(module_name, abs_path).load_module()

    if __PYTHON_35_PLUS:
        spec = importlib.util.spec_from_file_location(module_name, abs_path)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)

    if len(args) == 1:
        return module

    # Can't use a comprehension here since module wouldn't be in the
    # comprehensions scope for the eval
    returns = []
    for arg in args:
        returns.append(eval('module.{}'.format(arg)))
    return returns
