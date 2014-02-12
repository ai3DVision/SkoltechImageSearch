%% sifts from 1 folder
clc
clear all;
close all;

tic
Folder = '/mnt/Images/1/';
Imgs = dir(Folder);
ImgsSize = size(Imgs,1);

idxs = randsample(10000, 2000);
sifts = zeros(128, 10000);

count = 1;
i = 1;
while(count < 1000)
    try
        img = imread([Folder '/' Imgs(idxs(i)+2).name]);
        img = rgb2gray(img);
        [~, desc] = vl_sift(single(img));
        sifts(:,(count-1)*10+1:count*10) = vl_colsubset(desc, 10);
        count = count + 1;
        i = i + 1;
    catch exc
        i = i + 1;
    end
end
toc

save('sifts_1.mat', 'sifts');

%% vacabulary
load sifts_1.mat

[C, A] = vl_kmeans(sifts, 64);

%% sifts encodings from Holidays folder
tic
Folder = '/mnt/Images/Holidays/';
Imgs = dir(Folder);
ImgsSize = size(Imgs,1);

encs = zeros(8192, ImgsSize);

for i = 1:ImgsSize
    try
        if Imgs(i).isdir == 0
            img = imread([Folder '/' Imgs(i).name]);
            img = rgb2gray(img);
            [~, desc] = vl_sift(single(img));
            desc = double(desc);
            D = vl_alldist2(C,desc);
            encs(:,i) = vl_vlad(desc, C, D);
        end
        i/ImgsSize
    catch exc
    end
end
toc

encs(:,ImgsSize) = [];
encs(:,1) = [];
encs(:,1) = [];

save('encs_holidays.mat', 'encs');

%% query search
load encs_holidays.mat

Imgs = dir(Folder);

Imgs(ImgsSize) = [];
Imgs(1) = [];
Imgs(1) = [];

ImgsSize = size(Imgs,1);

for i = 2:ImgsSize+1
    if i == ImgsSize+1 || str2double(Imgs(i).name(end-5:end-4)) == 0
        imagesNum = str2double(Imgs(i-1).name(end-5:end-4)) + 1;
        idx_first =  i-imagesNum;
        % uncomment to display query results
%         D = vl_alldist2(encs,encs(:,idx_first));
%         [D_, sort_idxs] = sort(D);
%         figure
%         for k = 1:10
%             subplot(2,5,k);
%             img = imread([Folder '/' Imgs(sort_idxs(k)).name]);
%             subimage(img);
%             title(Imgs(sort_idxs(k)).name);
%             axis off;
%         end
%         pause
%         close all;
        
        % probability
        for j = idx_first:i-1
        end
    end
end













