#include "vptranslator/vptranslator.h"

void vptranslator::initcibles(){
}

MakeTranslation::MakeTranslation(){
}

bool MakeTranslation::initcs(boost::gregorian::date beginningday, std::string requestedcs){
  std::size_t found;

  found = requestedcs.find_last_of('1');
  //on recherche le dernier jour de validité de la condition de service
  if(found==std::string::npos){
    return false;
  } else {
      enddate = beginningday + boost::gregorian::date_duration(found);
      //on recherche le premier jour de validité
      found = requestedcs.find('1');
      startdate = beginningday + boost::gregorian::date_duration(found);
      CS = requestedcs.substr(found, (enddate - startdate).days() + 1);
      return true;
 }
}

int MakeTranslation::getnextmonday(boost::gregorian::date datetocompare, short sens){
    if (sens == 1){
        return (8 - datetocompare.day_of_week()) % 7;
    } else {
        return ((6 + datetocompare.day_of_week()) % 7) * -1;
    }
}

void MakeTranslation::splitcs(){
//    on recherche la position du premier lundi dans la chaine
    std::string substr;
    short int weeknumber = 0;
    //pos represente la position au sein de la condition de service
    short int pos = getnextmonday(startdate, 1);
    short int precpos = 0;

    while (precpos < CS.length()){
    //tant que le prochain lundi est inférieur à enddate, on découpe le régime
        if(pos < CS.length()){
            substr = CS.substr(precpos, pos - precpos);
        } else {
            substr = "0000000";
            substr.replace(0, CS.length() - precpos, CS.substr(precpos, CS.length() - precpos));
        }
        week_map[weeknumber].week_bs = std::bitset<7>(std::string(substr));
        weeknumber++;
        precpos = pos;
        pos = pos + 7;
    }
}
