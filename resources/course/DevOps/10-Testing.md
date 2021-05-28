We are in a DevOps section, so this means I have to talk about many people's least favorite thing... tests. Long story short, tests are how we ensure that our builds work when we make changes and that we aren't introducing any bugs or breaking any existing functionality. The way that we will do this in this course is with a python script that does various tests and outputs the results. 

We aren't going to go too crazy here and will just make sure individual things work nicely. 

To start, write a python script that will ensure there are no memory leaks using Valgrind, that you can compile and execute your implant successfully. If all checks pass, mark it as successful. As we go through the course we will add more tests, and you can and should add whatever you want. 

As you go through the course, ensure all of your implemented tests pass each time you do a pull request. If they don't pass, mention it in your commit. There's nothing terribly wrong with tests not passing, you just need to annotate it by creating an issue and getting it onto your worklist. As professionals working on what will eventually become a pretty large codebase, bugs are expected, and not everything will be able to be fixed. Write good tests, prioritize your fixes, and the world is yours. 

Submit your commit. 


