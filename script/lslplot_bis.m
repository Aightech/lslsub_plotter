addpath(genpath('liblsl-Matlab'));

disp('Loading the library...');
lib = lsl_loadlib();
version = lsl_library_version(lib);
disp(version);


% resolve a stream...
disp('Resolving an EEG stream...');
result = {};
while isempty(result)
    result = lsl_resolve_byprop(lib,'type','sample'); end

% create a new inlet
disp('Opening an inlet...');
inlet = lsl_inlet(result{1});

sound = zeros(200000)

disp('Now receiving data...');
figure; % open a new figure
while true 
    % get data from the inlet
    [vec,ts] = inlet.pull_sample();
    % and display it
    A = reshape(vec, [8,8]);
    surf(A);
    
    zlim([0,800]);
    pause(0.1);
    fprintf('%.2f\n',A(1,1));
%     fprintf('%.5f\n',ts);
end