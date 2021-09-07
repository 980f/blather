#to kill files in one branch that were deleted in another, supply the branches as below, without the git rm part until you are sure you have the direction of diff correct
git diff hadzombies...cleanbranch --name-status --diff-filter=D | cut -f2 | xargs -d "\n" git rm 
