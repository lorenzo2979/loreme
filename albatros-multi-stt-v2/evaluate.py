"""Offline ALBATROS transcript evaluator: synthetic and user-consented tests only.

JSON input: [{"expected":"accendi luce cucina",
              "outputs":[{"engine":"speech_to_phrase","text":"..."},
                         {"engine":"whisper_local","text":"..."}]}]
Never uploads audio or calls HA; no cloud calls. Does not log individual user speech.
"""
import argparse
import json
from agent import Result, judge, normalize

def score(items):
    results = {"total":len(items),"accepted":0,"correct":0,
               "wrong_commands":0,"abstentions":0,"wrong_transcripts":0}
    for item in items:
        expected=normalize(item["expected"])
        answers=[Result(str(x["engine"]),str(x["text"]))
                 for x in item["outputs"]]
        selection=judge(answers)
        if selection is None:
            results["abstentions"]+=1
            continue
        results["accepted"]+=1
        if normalize(selection.text)==expected:
            results["correct"]+=1
        else:
            results["wrong_transcripts"]+=1
            if expected.startswith(("accendi ","spegni ","apri ","chiudi ","attiva ","disattiva ")):
                results["wrong_commands"]+=1
    total=max(len(items),1)
    results["correct_pct"]=round(100*results["correct"]/total,1)
    results["abstention_pct"]=round(100*results["abstentions"]/total,1)
    return results

def main():
    p=argparse.ArgumentParser()
    p.add_argument("dataset",help="JSON of user-authorized test transcripts (not audio)")
    args=p.parse_args()
    with open(args.dataset,encoding="utf-8") as f:
        items=json.load(f)
    if not isinstance(items,list):
        p.error("Expected a list of test cases")
    print(json.dumps(score(items),ensure_ascii=False,indent=2))

if __name__=="__main__":
    main()
