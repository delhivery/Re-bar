from setuptools import setup, find_packages


setup(
    name='done-marg',
    description=(
        'A python package to find the shortest '
        'path across delhivery centers'),
    version='1.0.8',
    packages=find_packages(),

    author='Amit Prakash Ambasta',
    author_email='amit.ambasta@delhivery.com',

    namespace_packages=['done'],
    scripts=['bin/sampler', 'bin/tarjan'],
    install_requires=['bunch'],
    extras_require={
        'test': ['bunch', 'nose'],
        'scripts': ['click', 'progressbar', 'bunch'],
    },
    classifiers=[
        'Development Status :: 5 - Stable',
        'Intended Audience :: lm-tech@delhivery.com',
        'Topic :: Data Services :: Path Tools',
        'Programming Language :: Python :: 2.7',
        'Programming Language :: Python :: 3.4',
    ],
    test_suite='tests',
    keywords='done routing linehaul',
)
