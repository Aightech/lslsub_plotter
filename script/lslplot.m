addpath(genpath('liblsl-Matlab'));

disp('Loading the library...');
lib = lsl_loadlib();
version = lsl_library_version(lib);
disp(version);


% resolve a stream...
disp('Resolving an EEG stream...');
result = {};
while isempty(result)
    result = lsl_resolve_byprop(lib,'type','measurement'); end

% create a new inlet
disp('Opening an inlet...');
inlet = lsl_inlet(result{1});

disp('Now receiving data...');
%figure; % open a new figure
sig = zeros(200000,1);
tt = zeros(200000,1);
i=1
while i<200001 
    % get data from the inlet
    [vec,ts] = inlet.pull_sample();
    sig(i)=vec(1);
    tt(i)=ts;
    i=i+1
%     % and display it
%     A = reshape(vec, [8,8]);
%     surf(A);
%     
%     zlim([0,800]);
%     pause(0.1);
%     fprintf('%.2f\n',A(1,1));
% %     fprintf('%.5f\n',ts);
end

arr = [tt,sig];
arr2 = sortrows(arr);