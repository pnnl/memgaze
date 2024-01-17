def add_hot_lines (strFileName):
    dictRegions= {}
    dictTopLines={}
    strNewFile = strFileName+'_hot_lines'
    f_out=open(strNewFile,'w')
    with open(strFileName) as f:
        lineStart='---'
        lineEnd='<----'
        flRegions = True
        flLines= False
        flNewLine=False
        for fileLine in f:
            flNewLine=False
            if(fileLine.startswith(lineStart) and flRegions):
                data = fileLine.replace('\n','').split(' ')
                print(data[2], data[4])
                dictRegions[data[2]]= data[4]
            if (fileLine.startswith(lineEnd)):
                flRegions = False
            if (fileLine.startswith('#---- Top Access line')):
                data = fileLine.replace('\n','').split(' ')
                #print(data[5], data[11],data[13],data[15])
                print(str(data[11])+'-'+str(data[13])+'-'+str(data[15]))
                dictTopLines[data[11]+'-'+data[13]+'-'+data[15]] = str(data[5])
            if(fileLine.startswith('<---- New intra-region starts')):
                flLines=True
            if(flLines and len(fileLine.strip()) != 0):
                flNewLine=False
                newFileLine=''
                data = fileLine.replace('\n','').split(' ')
                #print(data)
                regionId=data[2][:-1]
                pageId = data[2][-1]
                if(regionId in dictRegions):
                    regPageId = dictRegions[regionId]+'-'+pageId
                    #print(regPageId)
                    listLines = [(key, value) for key, value in dictTopLines.items() if key.startswith(regPageId)]
                    if(len(listLines) !=0 ):
                        print('listlines ', listLines, regPageId, data[2], data[4])
                        hot_line_data= {}
                        for i in range(len(listLines)):
                            index=listLines[i][0].rindex('-')+1
                            print('list lines data ', listLines[i][0], listLines[i][0][index:])
                            hotBlock = listLines[i][0][index:]
                            if(str(hotBlock+',') in fileLine):
                                #print ('fileLine ', fileLine)
                                startIndex=fileLine.index(hotBlock+',')
                                lineSegment =fileLine[startIndex:]
                                startIndex = lineSegment.index(' ')
                                lineSegment = lineSegment[0:startIndex]
                                #print('lineSegment ', lineSegment)
                                cor_data=lineSegment.split(',')
                                print( 'cor_data ', regPageId+'-'+cor_data[0])
                                if (str(regPageId+'-'+cor_data[0]) in dictTopLines):
                                    hot_line_data[dictTopLines[str(regPageId+'-'+cor_data[0])]] = cor_data[1]
                                    #print('yes', cor_data[0], cor_data[1])
                                #else:
                                    #print("NO ", str(regPageId+'-'+cor_data[0]))
                        print('hot_line_data ',hot_line_data)
                        newFileLine=fileLine
                        print('newFileLine ', newFileLine)
                        for (key, value ) in hot_line_data.items():
                            nextBlock = 0
                            flIndex=False
                            while (True):
                                nextBlock = nextBlock+1
                                searchBlock = int(key)+nextBlock
                                searchBlock = str(searchBlock)+','
                                print(searchBlock)
                                if(newFileLine.find(searchBlock)):
                                    startIndex = newFileLine.find(searchBlock)
                                    if(startIndex != -1):
                                        print(key, value, newFileLine)
                                        newFileLine = newFileLine[0:startIndex-1] + ' '+key+','+value+ ' ' +newFileLine[startIndex:]
                                        #print(newFileLine)
                                        #print(listLines, regPageId, data[2], data[4])
                                        flNewLine=True
                                        break
                                if(int(key)+nextBlock >=267):
                                    break
                            print('after while ', startIndex)
            if(flNewLine == False):
                f_out.write(fileLine)
            else:
                f_out.write(newFileLine)

    f_out.close()
    f.close()
    print(dictTopLines)
    print(dictRegions)
add_hot_lines('/Users/suri836/Projects/spatial_rud/spatial_pages_exp/miniVite/hot_lines/v3_spatial_det.txt')
