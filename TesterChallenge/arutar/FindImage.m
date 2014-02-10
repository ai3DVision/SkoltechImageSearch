function [ imgIdx ] = FindImage( Folder, img )
%FINDIMAGE Summary of this function goes here
%   Detailed explanation goes here

Imgs = dir([Folder '/' '*.jpg']);
ImgsSize = size(Imgs,1);
sizeX = size(img,1);
sizeY = size(img,2);
imgIdx = 0;

for i = 1:ImgsSize
    img_ = double(imread([Folder '/' Imgs(i).name]));
    img_ = img_(1:sizeX, 1:sizeY, :) ./ 255;
    res = img_ == img;
    if (sum(res(:)) == sizeX * sizeY * 3)
        imgIdx = i;
        break;
    end
end

end

